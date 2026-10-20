/* pc_http.c - small blocking HTTP/1.1 client with HTTPS through mbedTLS (see pc_http.h) */
#include "pc_http.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/error.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/x509_crt.h"
#include "psa/crypto.h"

#define PC_HTTP_TIMEOUT_MS 15000
#define PC_HTTP_MAX_RESPONSE (8u * 1024u * 1024u)

typedef struct {
    int https;
    char host[256];
    char port[8];
    char path[512];
} PcHttpUrl;

static int s_tls_ready = 0;
static mbedtls_entropy_context s_entropy;
static mbedtls_ctr_drbg_context s_drbg;
static mbedtls_x509_crt s_ca;

static int pc_http_parse_url(const char* url, PcHttpUrl* u) {
    const char* p;
    const char* host_end;
    const char* colon;
    size_t host_len;

    memset(u, 0, sizeof(*u));
    if (strncmp(url, "https://", 8) == 0) {
        u->https = 1;
        p = url + 8;
        strcpy(u->port, "443");
    } else if (strncmp(url, "http://", 7) == 0) {
        p = url + 7;
        strcpy(u->port, "80");
    } else {
        return 0;
    }

    host_end = strchr(p, '/');
    if (host_end == NULL) {
        host_end = p + strlen(p);
    }
    colon = memchr(p, ':', host_end - p);
    host_len = (colon != NULL ? colon : host_end) - p;
    if (host_len == 0 || host_len >= sizeof(u->host)) {
        return 0;
    }
    memcpy(u->host, p, host_len);
    if (colon != NULL) {
        size_t port_len = host_end - colon - 1;
        if (port_len == 0 || port_len >= sizeof(u->port)) {
            return 0;
        }
        memcpy(u->port, colon + 1, port_len);
        u->port[port_len] = '\0';
    }
    snprintf(u->path, sizeof(u->path), "%s", *host_end != '\0' ? host_end : "/");
    return 1;
}

/* Root certificates: the system bundle on Linux, else cacert.pem next to the game. */
static int pc_http_tls_init(char* err, size_t err_size) {
    static const char* bundles[] = {
        "/etc/ssl/certs/ca-certificates.crt", /* Debian, Arch */
        "/etc/pki/tls/certs/ca-bundle.crt",   /* Fedora */
        "/etc/ssl/cert.pem",                  /* Alpine, macOS */
        "cacert.pem",                         /* shipped with the game (Windows) */
        NULL,
    };
    int i;
    int loaded = 0;

    if (s_tls_ready) {
        return 1;
    }
    if (psa_crypto_init() != PSA_SUCCESS) {
        snprintf(err, err_size, "psa_crypto_init failed");
        return 0;
    }
    mbedtls_entropy_init(&s_entropy);
    mbedtls_ctr_drbg_init(&s_drbg);
    mbedtls_x509_crt_init(&s_ca);
    if (mbedtls_ctr_drbg_seed(&s_drbg, mbedtls_entropy_func, &s_entropy, (const unsigned char*)"acgc", 4) != 0) {
        snprintf(err, err_size, "random generator init failed");
        return 0;
    }
    for (i = 0; bundles[i] != NULL; i++) {
        if (mbedtls_x509_crt_parse_file(&s_ca, bundles[i]) >= 0) {
            loaded = 1;
            break;
        }
    }
    if (!loaded) {
        snprintf(err, err_size, "no root certificates found (cacert.pem)");
        return 0;
    }
    s_tls_ready = 1;
    return 1;
}

typedef struct {
    mbedtls_net_context net;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    int https;
} PcHttpConn;

static int pc_http_send_all(PcHttpConn* c, const unsigned char* data, size_t len) {
    while (len > 0) {
        int n = c->https ? mbedtls_ssl_write(&c->ssl, data, len) : mbedtls_net_send(&c->net, data, len);

        if (n == MBEDTLS_ERR_SSL_WANT_READ || n == MBEDTLS_ERR_SSL_WANT_WRITE) {
            continue;
        }
        if (n <= 0) {
            return 0;
        }
        data += n;
        len -= (size_t)n;
    }
    return 1;
}

static int pc_http_recv(PcHttpConn* c, unsigned char* buf, size_t len) {
    for (;;) {
        int n = c->https ? mbedtls_ssl_read(&c->ssl, buf, len)
                         : mbedtls_net_recv_timeout(&c->net, buf, len, PC_HTTP_TIMEOUT_MS);

        if (n == MBEDTLS_ERR_SSL_WANT_READ || n == MBEDTLS_ERR_SSL_WANT_WRITE) {
            continue;
        }
        if (n == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
            return 0;
        }
        return n; /* >0 data, 0 closed, <0 error */
    }
}

static const char* pc_http_find_header(const char* headers, const char* name) {
    size_t name_len = strlen(name);
    const char* line = headers;

    while (line != NULL && *line != '\0') {
        if (strncasecmp(line, name, name_len) == 0 && line[name_len] == ':') {
            const char* v = line + name_len + 1;
            while (*v == ' ') {
                v++;
            }
            return v;
        }
        line = strstr(line, "\r\n");
        if (line != NULL) {
            line += 2;
        }
    }
    return NULL;
}

/* Decode a chunked body in place; returns the new length, or -1 if malformed. */
static long pc_http_dechunk(uint8_t* data, uint32_t len) {
    uint32_t in = 0;
    uint32_t out = 0;

    for (;;) {
        char* end;
        unsigned long size;
        uint8_t* line_end = (uint8_t*)strstr((char*)data + in, "\r\n");

        if (line_end == NULL) {
            return -1;
        }
        size = strtoul((char*)data + in, &end, 16);
        in = (uint32_t)(line_end - data) + 2;
        if (size == 0) {
            return out;
        }
        if (in + size > len) {
            return -1;
        }
        memmove(data + out, data + in, size);
        out += size;
        in += size + 2; /* chunk + CRLF */
    }
}

static int pc_http_parse_response(uint8_t* raw, uint32_t raw_len, PcHttpResponse* out) {
    char* header_end;
    uint32_t head_len;
    const char* v;
    uint8_t* body;
    uint32_t body_len;

    if (raw_len < 12 || strncmp((char*)raw, "HTTP/1.", 7) != 0) {
        snprintf(out->error, sizeof(out->error), "invalid response from server");
        return 0;
    }
    raw[raw_len] = '\0'; /* buffer has room for it */
    header_end = strstr((char*)raw, "\r\n\r\n");
    if (header_end == NULL) {
        snprintf(out->error, sizeof(out->error), "truncated response");
        return 0;
    }
    header_end[2] = '\0'; /* keep the last header's CRLF for the header scan */
    head_len = (uint32_t)(header_end - (char*)raw) + 4;
    out->status = atoi((char*)raw + 9);

    v = pc_http_find_header((char*)raw, "X-Save-Version");
    if (v != NULL) {
        size_t n = strcspn(v, "\r\n");
        if (n >= sizeof(out->save_version)) {
            n = sizeof(out->save_version) - 1;
        }
        memcpy(out->save_version, v, n);
        out->save_version[n] = '\0';
    }

    body = raw + head_len;
    body_len = raw_len - head_len;
    v = pc_http_find_header((char*)raw, "Transfer-Encoding");
    if (v != NULL && strncasecmp(v, "chunked", 7) == 0) {
        long n;

        body[body_len] = '\0';
        n = pc_http_dechunk(body, body_len);
        if (n < 0) {
            snprintf(out->error, sizeof(out->error), "malformed chunked response");
            out->status = 0;
            return 0;
        }
        body_len = (uint32_t)n;
    } else if ((v = pc_http_find_header((char*)raw, "Content-Length")) != NULL) {
        unsigned long n = strtoul(v, NULL, 10);
        if (n > body_len) {
            snprintf(out->error, sizeof(out->error), "response cut short");
            out->status = 0;
            return 0;
        }
        body_len = (uint32_t)n;
    }

    out->body = (uint8_t*)malloc(body_len + 1);
    if (out->body == NULL) {
        out->status = 0;
        return 0;
    }
    memcpy(out->body, body, body_len);
    out->body[body_len] = '\0'; /* JSON bodies can be used as C strings */
    out->body_len = body_len;
    return 1;
}

int pc_http_request(const char* method, const char* url, const char* const* headers, const void* body,
                    uint32_t body_len, PcHttpResponse* out) {
    PcHttpUrl u;
    PcHttpConn c;
    char head[2048];
    int head_len;
    int i;
    uint8_t* raw = NULL;
    uint32_t raw_len = 0;
    uint32_t raw_cap = 0;
    int ok = 0;
    int ret;

    memset(out, 0, sizeof(*out));
    if (!pc_http_parse_url(url, &u)) {
        snprintf(out->error, sizeof(out->error), "bad server address");
        return 0;
    }
    if (u.https && !pc_http_tls_init(out->error, sizeof(out->error))) {
        return 0;
    }

    memset(&c, 0, sizeof(c));
    c.https = u.https;
    mbedtls_net_init(&c.net);
    mbedtls_ssl_init(&c.ssl);
    mbedtls_ssl_config_init(&c.conf);

    if ((ret = mbedtls_net_connect(&c.net, u.host, u.port, MBEDTLS_NET_PROTO_TCP)) != 0) {
        snprintf(out->error, sizeof(out->error), "could not reach %.100s", u.host);
        goto done;
    }

    if (u.https) {
        mbedtls_ssl_config_defaults(&c.conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM,
                                    MBEDTLS_SSL_PRESET_DEFAULT);
        mbedtls_ssl_conf_authmode(&c.conf, MBEDTLS_SSL_VERIFY_REQUIRED);
        mbedtls_ssl_conf_ca_chain(&c.conf, &s_ca, NULL);
        mbedtls_ssl_conf_rng(&c.conf, mbedtls_ctr_drbg_random, &s_drbg);
        mbedtls_ssl_conf_read_timeout(&c.conf, PC_HTTP_TIMEOUT_MS);
        if (mbedtls_ssl_setup(&c.ssl, &c.conf) != 0 || mbedtls_ssl_set_hostname(&c.ssl, u.host) != 0) {
            snprintf(out->error, sizeof(out->error), "TLS setup failed");
            goto done;
        }
        mbedtls_ssl_set_bio(&c.ssl, &c.net, mbedtls_net_send, NULL, mbedtls_net_recv_timeout);
        while ((ret = mbedtls_ssl_handshake(&c.ssl)) != 0) {
            if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
                char msg[96];
                mbedtls_strerror(ret, msg, sizeof(msg));
                snprintf(out->error, sizeof(out->error), "secure connection failed: %s", msg);
                goto done;
            }
        }
    }

    head_len = snprintf(head, sizeof(head),
                        "%s %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: acgc-online\r\nConnection: close\r\n"
                        "Content-Length: %u\r\n",
                        method, u.path, u.host, (unsigned)body_len);
    for (i = 0; headers != NULL && headers[i] != NULL; i++) {
        head_len += snprintf(head + head_len, sizeof(head) - head_len, "%s\r\n", headers[i]);
    }
    head_len += snprintf(head + head_len, sizeof(head) - head_len, "\r\n");
    if (head_len >= (int)sizeof(head) || !pc_http_send_all(&c, (const unsigned char*)head, head_len) ||
        (body_len > 0 && !pc_http_send_all(&c, (const unsigned char*)body, body_len))) {
        snprintf(out->error, sizeof(out->error), "could not send request");
        goto done;
    }

    /* Connection: close, so the response ends when the server closes. */
    for (;;) {
        int n;

        if (raw_cap - raw_len < 16 * 1024 + 2) {
            uint8_t* p;
            raw_cap = raw_cap ? raw_cap * 2 : 64 * 1024;
            if (raw_cap > PC_HTTP_MAX_RESPONSE) {
                snprintf(out->error, sizeof(out->error), "response too large");
                goto done;
            }
            p = (uint8_t*)realloc(raw, raw_cap);
            if (p == NULL) {
                goto done;
            }
            raw = p;
        }
        n = pc_http_recv(&c, raw + raw_len, raw_cap - raw_len - 2);
        if (n == 0) {
            break;
        }
        if (n < 0) {
            if (raw_len > 0 && n == MBEDTLS_ERR_SSL_CONN_EOF) {
                break;
            }
            snprintf(out->error, sizeof(out->error), "connection lost while reading");
            goto done;
        }
        raw_len += (uint32_t)n;
    }

    ok = pc_http_parse_response(raw, raw_len, out);

done:
    if (u.https) {
        mbedtls_ssl_close_notify(&c.ssl);
    }
    mbedtls_ssl_free(&c.ssl);
    mbedtls_ssl_config_free(&c.conf);
    mbedtls_net_free(&c.net);
    free(raw);
    if (!ok) {
        out->status = 0;
    }
    return ok;
}

void pc_http_free(PcHttpResponse* resp) {
    free(resp->body);
    resp->body = NULL;
    resp->body_len = 0;
}
