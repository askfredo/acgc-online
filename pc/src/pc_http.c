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

#include <SDL.h>

#define PC_HTTP_TIMEOUT_MS 15000
#define PC_HTTP_MAX_RESPONSE (8u * 1024u * 1024u)

typedef struct {
    int https;
    char host[256];
    char port[8];
    char path[512];
} PcHttpUrl;

static int s_tls_ready = 0;
static SDL_mutex* s_tls_lock = NULL; /* uploads run on their own thread: guard init and the RNG */
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

static int pc_http_rng(void* ctx, unsigned char* out, size_t len) {
    int ret;

    SDL_LockMutex(s_tls_lock);
    ret = mbedtls_ctr_drbg_random(ctx, out, len);
    SDL_UnlockMutex(s_tls_lock);
    return ret;
}

static int pc_http_tls_init_locked(char* err, size_t err_size);

/* Root certificates: the system bundle on Linux, else cacert.pem next to the game. */
static int pc_http_tls_init(char* err, size_t err_size) {
    int ok;

    if (s_tls_lock == NULL) {
        s_tls_lock = SDL_CreateMutex(); /* first call comes from the main thread at boot */
    }
    SDL_LockMutex(s_tls_lock);
    ok = pc_http_tls_init_locked(err, err_size);
    SDL_UnlockMutex(s_tls_lock);
    return ok;
}

static int pc_http_tls_init_locked(char* err, size_t err_size) {
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

/* TCP connection to the server, with TLS for https (blocking, with read timeouts). */
static int pc_http_conn_open(PcHttpConn* c, const PcHttpUrl* u, char* err, size_t err_size) {
    int ret;

    memset(c, 0, sizeof(*c));
    c->https = u->https;
    mbedtls_net_init(&c->net);
    mbedtls_ssl_init(&c->ssl);
    mbedtls_ssl_config_init(&c->conf);

    if (mbedtls_net_connect(&c->net, u->host, u->port, MBEDTLS_NET_PROTO_TCP) != 0) {
        snprintf(err, err_size, "could not reach %.100s", u->host);
        return 0;
    }
    if (!u->https) {
        return 1;
    }

    mbedtls_ssl_config_defaults(&c->conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM,
                                MBEDTLS_SSL_PRESET_DEFAULT);
    mbedtls_ssl_conf_authmode(&c->conf, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_ca_chain(&c->conf, &s_ca, NULL);
    mbedtls_ssl_conf_rng(&c->conf, pc_http_rng, &s_drbg);
    mbedtls_ssl_conf_read_timeout(&c->conf, PC_HTTP_TIMEOUT_MS);
    if (mbedtls_ssl_setup(&c->ssl, &c->conf) != 0 || mbedtls_ssl_set_hostname(&c->ssl, u->host) != 0) {
        snprintf(err, err_size, "TLS setup failed");
        return 0;
    }
    mbedtls_ssl_set_bio(&c->ssl, &c->net, mbedtls_net_send, NULL, mbedtls_net_recv_timeout);
    while ((ret = mbedtls_ssl_handshake(&c->ssl)) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            char msg[96];
            mbedtls_strerror(ret, msg, sizeof(msg));
            snprintf(err, err_size, "secure connection failed: %s", msg);
            return 0;
        }
    }
    return 1;
}

static void pc_http_conn_close(PcHttpConn* c) {
    if (c->https) {
        mbedtls_ssl_close_notify(&c->ssl);
    }
    mbedtls_ssl_free(&c->ssl);
    mbedtls_ssl_config_free(&c->conf);
    mbedtls_net_free(&c->net);
}

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

    memset(out, 0, sizeof(*out));
    if (!pc_http_parse_url(url, &u)) {
        snprintf(out->error, sizeof(out->error), "bad server address");
        return 0;
    }
    if (u.https && !pc_http_tls_init(out->error, sizeof(out->error))) {
        return 0;
    }

    if (!pc_http_conn_open(&c, &u, out->error, sizeof(out->error))) {
        goto done;
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
    pc_http_conn_close(&c);
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

/* --- WebSocket client (RFC 6455): masked binary frames out, server frames in --- */

#define PC_WS_MAX_MESSAGE (4u * 1024u * 1024u)
#define PC_WS_CHUNK 16384u

typedef struct {
    uint8_t* data;
    uint32_t len;
    uint32_t cap;
} PcWsBuf;

struct PcWs {
    PcHttpConn conn;
    PcWsBuf in;
    PcWsBuf out;
    uint32_t pending_chunk; /* a write that would block must be retried with the same length */
    uint32_t mask_seed;
    int closed;
};

static int pc_ws_buf_put(PcWsBuf* b, const void* data, uint32_t len) {
    if (b->len + len > b->cap) {
        uint32_t cap = b->cap ? b->cap : 64 * 1024;
        uint8_t* p;

        while (cap < b->len + len) {
            cap *= 2;
        }
        p = (uint8_t*)realloc(b->data, cap);
        if (p == NULL) {
            return 0;
        }
        b->data = p;
        b->cap = cap;
    }
    if (data != NULL) {
        memcpy(b->data + b->len, data, len);
    }
    b->len += len;
    return 1;
}

static void pc_ws_buf_take(PcWsBuf* b, uint32_t n) {
    memmove(b->data, b->data + n, b->len - n);
    b->len -= n;
}

static int pc_ws_queue_frame(PcWs* ws, int op, const void* data, uint32_t len) {
    uint8_t hdr[14];
    uint32_t n = 2;
    uint8_t mask[4];
    uint32_t i;
    uint8_t* dst;

    hdr[0] = (uint8_t)(0x80 | op);
    if (len < 126) {
        hdr[1] = (uint8_t)(0x80 | len);
    } else if (len <= 0xFFFF) {
        hdr[1] = 0x80 | 126;
        hdr[2] = (uint8_t)(len >> 8);
        hdr[3] = (uint8_t)len;
        n = 4;
    } else {
        hdr[1] = 0x80 | 127;
        memset(hdr + 2, 0, 4);
        hdr[6] = (uint8_t)(len >> 24);
        hdr[7] = (uint8_t)(len >> 16);
        hdr[8] = (uint8_t)(len >> 8);
        hdr[9] = (uint8_t)len;
        n = 10;
    }
    /* The mask only stops proxies from mistaking frames for HTTP; it needn't be secret. */
    ws->mask_seed = ws->mask_seed * 1664525u + 1013904223u;
    memcpy(mask, &ws->mask_seed, 4);
    memcpy(hdr + n, mask, 4);
    n += 4;

    if (!pc_ws_buf_put(&ws->out, hdr, n) || !pc_ws_buf_put(&ws->out, NULL, len)) {
        return 0;
    }
    dst = ws->out.data + ws->out.len - len;
    for (i = 0; i < len; i++) {
        dst[i] = ((const uint8_t*)data)[i] ^ mask[i & 3];
    }
    return 1;
}

PcWs* pc_ws_connect(const char* url, const char* const* headers, char* err, int err_size) {
    PcHttpUrl u;
    PcWs* ws;
    char head[2048];
    int head_len;
    int i;
    uint32_t got = 0;
    char* end;

    if (!pc_http_parse_url(url, &u)) {
        snprintf(err, err_size, "bad server address");
        return NULL;
    }
    if (u.https && !pc_http_tls_init(err, err_size)) {
        return NULL;
    }
    ws = (PcWs*)calloc(1, sizeof(PcWs));
    if (ws == NULL) {
        return NULL;
    }
    ws->mask_seed = SDL_GetTicks() ^ 0x5EED1234u;
    if (!pc_http_conn_open(&ws->conn, &u, err, err_size)) {
        pc_http_conn_close(&ws->conn);
        free(ws);
        return NULL;
    }

    head_len = snprintf(head, sizeof(head),
                        "GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: acgc-online\r\nUpgrade: websocket\r\n"
                        "Connection: Upgrade\r\nSec-WebSocket-Version: 13\r\n"
                        "Sec-WebSocket-Key: YWNnYy1vbmxpbmUtbGl2ZQ==\r\n",
                        u.path, u.host);
    for (i = 0; headers != NULL && headers[i] != NULL; i++) {
        head_len += snprintf(head + head_len, sizeof(head) - head_len, "%s\r\n", headers[i]);
    }
    head_len += snprintf(head + head_len, sizeof(head) - head_len, "\r\n");
    if (head_len >= (int)sizeof(head) || !pc_http_send_all(&ws->conn, (const unsigned char*)head, head_len)) {
        snprintf(err, err_size, "could not send request");
        goto fail;
    }

    /* The handshake answer (blocking); anything after it is already WebSocket data. */
    for (;;) {
        int n;

        if (ws->in.cap - ws->in.len < 1024) {
            uint32_t len = ws->in.len;

            if (!pc_ws_buf_put(&ws->in, NULL, 4096)) {
                goto fail;
            }
            ws->in.len = len; /* only the room was wanted */
        }
        n = pc_http_recv(&ws->conn, ws->in.data + ws->in.len, ws->in.cap - ws->in.len - 1);
        if (n <= 0) {
            snprintf(err, err_size, "connection closed during the handshake");
            goto fail;
        }
        ws->in.len += (uint32_t)n;
        ws->in.data[ws->in.len] = '\0';
        end = strstr((char*)ws->in.data, "\r\n\r\n");
        if (end != NULL) {
            got = (uint32_t)(end - (char*)ws->in.data) + 4;
            break;
        }
        if (ws->in.len > 8192) {
            snprintf(err, err_size, "bad handshake answer");
            goto fail;
        }
    }
    if (strncmp((char*)ws->in.data, "HTTP/1.1 101", 12) != 0) {
        /* A refusal: pass the server's {"error": ...} along. */
        char* msg = strstr((char*)ws->in.data, "\"error\":\"");
        if (msg != NULL) {
            size_t n = strcspn(msg + 9, "\"");
            snprintf(err, err_size, "%.*s", (int)(n < 120 ? n : 120), msg + 9);
        } else {
            snprintf(err, err_size, "the server refused the live connection");
        }
        goto fail;
    }
    pc_ws_buf_take(&ws->in, got);

    /* From here on nothing blocks. */
    mbedtls_net_set_nonblock(&ws->conn.net);
    if (u.https) {
        mbedtls_ssl_set_bio(&ws->conn.ssl, &ws->conn.net, mbedtls_net_send, mbedtls_net_recv, NULL);
    }
    return ws;

fail:
    pc_http_conn_close(&ws->conn);
    free(ws->in.data);
    free(ws);
    return NULL;
}

int pc_ws_send(PcWs* ws, const void* data, uint32_t len) {
    return ws != NULL && !ws->closed && len <= PC_WS_MAX_MESSAGE && pc_ws_queue_frame(ws, PC_WS_BINARY, data, len);
}

static void pc_ws_flush(PcWs* ws) {
    while (!ws->closed && ws->out.len > 0) {
        uint32_t chunk = ws->pending_chunk ? ws->pending_chunk : (ws->out.len < PC_WS_CHUNK ? ws->out.len : PC_WS_CHUNK);
        int n = ws->conn.https ? mbedtls_ssl_write(&ws->conn.ssl, ws->out.data, chunk)
                               : mbedtls_net_send(&ws->conn.net, ws->out.data, chunk);

        if (n == MBEDTLS_ERR_SSL_WANT_WRITE || n == MBEDTLS_ERR_SSL_WANT_READ) {
            ws->pending_chunk = chunk;
            return;
        }
        if (n <= 0) {
            ws->closed = 1;
            return;
        }
        ws->pending_chunk = 0;
        pc_ws_buf_take(&ws->out, (uint32_t)n);
    }
}

static void pc_ws_receive(PcWs* ws) {
    while (!ws->closed) {
        int n;

        if (ws->in.cap - ws->in.len < PC_WS_CHUNK) {
            uint32_t len = ws->in.len;
            if (!pc_ws_buf_put(&ws->in, NULL, PC_WS_CHUNK)) {
                ws->closed = 1;
                return;
            }
            ws->in.len = len;
        }
        n = ws->conn.https ? mbedtls_ssl_read(&ws->conn.ssl, ws->in.data + ws->in.len, ws->in.cap - ws->in.len)
                           : mbedtls_net_recv(&ws->conn.net, ws->in.data + ws->in.len, ws->in.cap - ws->in.len);
        if (n == MBEDTLS_ERR_SSL_WANT_READ || n == MBEDTLS_ERR_SSL_WANT_WRITE) {
            return;
        }
        if (n <= 0) {
            ws->closed = 1;
            return;
        }
        ws->in.len += (uint32_t)n;
    }
}

int pc_ws_poll(PcWs* ws, PcWsMessageFn on_message, void* user) {
    if (ws == NULL) {
        return 0;
    }
    pc_ws_flush(ws);
    pc_ws_receive(ws);

    /* Whole frames from the server (never masked, never fragmented by our relay). */
    while (ws->in.len >= 2) {
        const uint8_t* p = ws->in.data;
        int op = p[0] & 0x0F;
        uint64_t len = p[1] & 0x7F;
        uint32_t hdr = 2;

        if (len == 126) {
            if (ws->in.len < 4) break;
            len = ((uint32_t)p[2] << 8) | p[3];
            hdr = 4;
        } else if (len == 127) {
            if (ws->in.len < 10) break;
            len = ((uint64_t)p[6] << 24) | ((uint64_t)p[7] << 16) | ((uint64_t)p[8] << 8) | p[9];
            hdr = 10;
        }
        if (len > PC_WS_MAX_MESSAGE) {
            ws->closed = 1;
            break;
        }
        if (ws->in.len < hdr + len) {
            break;
        }

        if (op == 0x9) { /* ping */
            pc_ws_queue_frame(ws, 0xA, p + hdr, (uint32_t)len);
        } else if (op == 0x8) { /* close */
            ws->closed = 1;
        } else if ((op == PC_WS_TEXT || op == PC_WS_BINARY) && on_message != NULL) {
            on_message(op, p + hdr, (uint32_t)len, user);
        }
        pc_ws_buf_take(&ws->in, hdr + (uint32_t)len);
    }
    pc_ws_flush(ws);
    return !ws->closed;
}

void pc_ws_close(PcWs* ws) {
    if (ws == NULL) {
        return;
    }
    if (!ws->closed) {
        pc_ws_queue_frame(ws, 0x8, NULL, 0);
        pc_ws_flush(ws);
    }
    pc_http_conn_close(&ws->conn);
    free(ws->in.data);
    free(ws->out.data);
    free(ws);
}
