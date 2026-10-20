/* pc_http.h - small blocking HTTP/1.1 client (HTTPS through mbedTLS) for the town server.
 *
 * Not thread-safe: all requests go through one thread (the boot code, then the
 * cloud worker in pc_cloud.c). */
#ifndef PC_HTTP_H
#define PC_HTTP_H

#include <stdint.h>

typedef struct {
    int status; /* HTTP status, or 0 if the request failed (see error) */
    uint8_t* body;
    uint32_t body_len;
    char save_version[16]; /* X-Save-Version header, if any */
    char error[128];
} PcHttpResponse;

/* headers: NULL-terminated list of "Name: value" strings (may be NULL).
 * Returns 1 if a response was received (any status), 0 on network/TLS errors. */
int pc_http_request(const char* method, const char* url, const char* const* headers, const void* body,
                    uint32_t body_len, PcHttpResponse* out);
void pc_http_free(PcHttpResponse* resp);

/* --- WebSocket (the town server's live relay) ---
 * Connects (blocking, then the socket is non-blocking); sends are queued and written out
 * by pc_ws_poll, which also delivers received messages. One thread only. */
typedef struct PcWs PcWs;

enum { PC_WS_TEXT = 1, PC_WS_BINARY = 2 };

typedef void (*PcWsMessageFn)(int op, const uint8_t* data, uint32_t len, void* user);

PcWs* pc_ws_connect(const char* url, const char* const* headers, char* err, int err_size);
int pc_ws_send(PcWs* ws, const void* data, uint32_t len); /* one binary message */
/* Flush queued sends and deliver incoming messages. Returns 0 once the connection is gone. */
int pc_ws_poll(PcWs* ws, PcWsMessageFn on_message, void* user);
void pc_ws_close(PcWs* ws);

#endif /* PC_HTTP_H */
