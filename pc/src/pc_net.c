/* pc_net.c - netplay TCP transport (see pc_net.h) */
#include "pc_net.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET pc_sock_t;
#define PC_SOCK_INVALID INVALID_SOCKET
#define pc_sock_close closesocket
#define pc_sock_would_block() (WSAGetLastError() == WSAEWOULDBLOCK)
#else
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int pc_sock_t;
#define PC_SOCK_INVALID (-1)
#define pc_sock_close close
#define pc_sock_would_block() (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINPROGRESS)
#endif

#include <SDL.h>

#define PC_NET_MAX_MSG (4u * 1024u * 1024u)
#define PC_NET_CONNECT_TIMEOUT_MS 30000
#define PC_NET_BOOT_SAVE_TIMEOUT_MS 60000

typedef struct {
    uint32_t len; /* payload bytes */
    uint16_t type;
    uint16_t pad;
} PcNetHeader;

typedef struct {
    uint8_t* data;
    uint32_t len;
    uint32_t cap;
} PcNetBuf;

int g_pc_net_mode = PC_NET_MODE_OFF;
char g_pc_net_addr[64] = "";
int g_pc_net_port = PC_NET_DEFAULT_PORT;

static pc_sock_t s_listen = PC_SOCK_INVALID;
static pc_sock_t s_conn = PC_SOCK_INVALID;
static PcNetBuf s_in;
static PcNetBuf s_out;
static int s_booting = 0; /* guest: capture SAVE into s_boot_save instead of dispatching */
static uint8_t* s_boot_save = NULL;
static uint32_t s_boot_save_size = 0;

static int pc_net_buf_reserve(PcNetBuf* b, uint32_t extra) {
    if (b->len + extra > b->cap) {
        uint32_t cap = b->cap ? b->cap : 4096;
        uint8_t* p;

        while (cap < b->len + extra) {
            cap *= 2;
        }
        p = (uint8_t*)realloc(b->data, cap);
        if (p == NULL) {
            return 0;
        }
        b->data = p;
        b->cap = cap;
    }
    return 1;
}

static void pc_net_buf_consume(PcNetBuf* b, uint32_t n) {
    memmove(b->data, b->data + n, b->len - n);
    b->len -= n;
}

static int pc_net_set_nonblocking(pc_sock_t s) {
#ifdef _WIN32
    u_long on = 1;
    return ioctlsocket(s, FIONBIO, &on) == 0;
#else
    int flags = fcntl(s, F_GETFL, 0);
    return flags >= 0 && fcntl(s, F_SETFL, flags | O_NONBLOCK) == 0;
#endif
}

static void pc_net_tune_conn(pc_sock_t s) {
    int on = 1;
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char*)&on, sizeof(on));
    pc_net_set_nonblocking(s);
}

static void pc_net_drop_conn(const char* why) {
    if (s_conn == PC_SOCK_INVALID) {
        return;
    }
    printf("[Net] connection closed (%s)\n", why);
    pc_sock_close(s_conn);
    s_conn = PC_SOCK_INVALID;
    s_in.len = 0;
    s_out.len = 0;
    if (!s_booting) {
        mNtp_on_disconnect();
    }
}

uint32_t pc_net_ticks_ms(void) {
    return SDL_GetTicks();
}

void pc_net_sleep_ms(int ms) {
    SDL_Delay(ms);
}

int pc_net_connected(void) {
    return s_conn != PC_SOCK_INVALID;
}

int pc_net_send(int type, const void* data, uint32_t len) {
    PcNetHeader hdr;

    if (s_conn == PC_SOCK_INVALID || len > PC_NET_MAX_MSG) {
        return 0;
    }
    if (!pc_net_buf_reserve(&s_out, sizeof(hdr) + len)) {
        return 0;
    }
    hdr.len = len;
    hdr.type = (uint16_t)type;
    hdr.pad = 0;
    memcpy(s_out.data + s_out.len, &hdr, sizeof(hdr));
    if (len > 0) {
        memcpy(s_out.data + s_out.len + sizeof(hdr), data, len);
    }
    s_out.len += sizeof(hdr) + len;
    return 1;
}

static void pc_net_flush(void) {
    while (s_conn != PC_SOCK_INVALID && s_out.len > 0) {
        int n = (int)send(s_conn, (const char*)s_out.data, (int)s_out.len, 0);

        if (n > 0) {
            pc_net_buf_consume(&s_out, (uint32_t)n);
        } else if (n < 0 && pc_sock_would_block()) {
            break;
        } else {
            pc_net_drop_conn("send failed");
        }
    }
}

static void pc_net_dispatch(void) {
    while (s_conn != PC_SOCK_INVALID && s_in.len >= sizeof(PcNetHeader)) {
        PcNetHeader hdr;

        memcpy(&hdr, s_in.data, sizeof(hdr));
        if (hdr.len > PC_NET_MAX_MSG) {
            pc_net_drop_conn("oversized message");
            return;
        }
        if (s_in.len < sizeof(hdr) + hdr.len) {
            return;
        }

        if (s_booting && hdr.type == PC_NET_MSG_SAVE) {
            free(s_boot_save);
            s_boot_save = (uint8_t*)malloc(hdr.len);
            if (s_boot_save != NULL) {
                memcpy(s_boot_save, s_in.data + sizeof(hdr), hdr.len);
                s_boot_save_size = hdr.len;
            }
        } else if (!s_booting) {
            mNtp_on_message(hdr.type, s_in.data + sizeof(hdr), hdr.len);
        }
        pc_net_buf_consume(&s_in, sizeof(hdr) + hdr.len);
    }
}

static void pc_net_receive(void) {
    while (s_conn != PC_SOCK_INVALID) {
        int n;

        if (!pc_net_buf_reserve(&s_in, 64 * 1024)) {
            pc_net_drop_conn("out of memory");
            return;
        }
        n = (int)recv(s_conn, (char*)s_in.data + s_in.len, 64 * 1024, 0);
        if (n > 0) {
            s_in.len += (uint32_t)n;
        } else if (n < 0 && pc_sock_would_block()) {
            break;
        } else {
            pc_net_drop_conn(n == 0 ? "peer left" : "recv failed");
            return;
        }
    }
    pc_net_dispatch();
}

static void pc_net_accept(void) {
    struct sockaddr_in from;
    socklen_t from_len = sizeof(from);
    pc_sock_t s = accept(s_listen, (struct sockaddr*)&from, &from_len);

    if (s == PC_SOCK_INVALID) {
        return;
    }
    if (s_conn != PC_SOCK_INVALID) {
        /* One guest at a time. */
        pc_sock_close(s);
        return;
    }
    pc_net_tune_conn(s);
    s_conn = s;
    s_in.len = 0;
    s_out.len = 0;
    printf("[Net] guest connected from %s\n", inet_ntoa(from.sin_addr));
    mNtp_on_connect();
}

void pc_net_poll(void) {
    if (g_pc_net_mode == PC_NET_MODE_OFF) {
        return;
    }
    if (s_listen != PC_SOCK_INVALID) {
        pc_net_accept();
    }
    pc_net_receive();
    if (s_conn != PC_SOCK_INVALID) {
        mNtp_tick();
    }
    pc_net_flush();
}

static void pc_net_fail(const char* msg) {
    fprintf(stderr, "[Net] %s\n", msg);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Animal Crossing - Multijugador", msg, NULL);
    exit(1);
}

static void pc_net_init_host(void) {
    struct sockaddr_in addr;
    int on = 1;

    s_listen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s_listen == PC_SOCK_INVALID) {
        pc_net_fail("No se pudo crear el socket del anfitrion.");
    }
    setsockopt(s_listen, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((uint16_t)g_pc_net_port);
    if (bind(s_listen, (struct sockaddr*)&addr, sizeof(addr)) != 0 || listen(s_listen, 1) != 0) {
        pc_net_fail("No se pudo abrir el puerto del anfitrion (ya hay otro juego usandolo?).");
    }
    pc_net_set_nonblocking(s_listen);
    printf("[Net] hosting on TCP port %d\n", g_pc_net_port);
}

static void pc_net_init_join(void) {
    struct addrinfo hints;
    struct addrinfo* res = NULL;
    struct sockaddr_in addr;
    uint32_t start = SDL_GetTicks();
    uint32_t version = PC_NET_PROTOCOL_VERSION;
    char msg[256];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(g_pc_net_addr, NULL, &hints, &res) != 0 || res == NULL) {
        snprintf(msg, sizeof(msg), "No se encontro el anfitrion '%s'.", g_pc_net_addr);
        pc_net_fail(msg);
    }
    memcpy(&addr, res->ai_addr, sizeof(addr));
    addr.sin_port = htons((uint16_t)g_pc_net_port);
    freeaddrinfo(res);

    printf("[Net] connecting to %s:%d\n", g_pc_net_addr, g_pc_net_port);
    /* Blocking connect with retries: the host may still be starting. */
    while (s_conn == PC_SOCK_INVALID) {
        pc_sock_t s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (s != PC_SOCK_INVALID && connect(s, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            pc_net_tune_conn(s);
            s_conn = s;
            break;
        }
        if (s != PC_SOCK_INVALID) {
            pc_sock_close(s);
        }
        if (SDL_GetTicks() - start > PC_NET_CONNECT_TIMEOUT_MS) {
            snprintf(msg, sizeof(msg),
                     "No se pudo conectar a %s:%d.\nRevisa que el anfitrion tenga el juego abierto con --host.",
                     g_pc_net_addr, g_pc_net_port);
            pc_net_fail(msg);
        }
        SDL_Delay(500);
    }

    printf("[Net] connected, downloading town...\n");
    s_booting = 1;
    pc_net_send(PC_NET_MSG_HELLO, &version, sizeof(version));
    start = SDL_GetTicks();
    while (s_boot_save == NULL) {
        pc_net_receive();
        pc_net_flush();
        if (s_conn == PC_SOCK_INVALID) {
            pc_net_fail("El anfitrion cerro la conexion antes de mandar el pueblo.");
        }
        if (SDL_GetTicks() - start > PC_NET_BOOT_SAVE_TIMEOUT_MS) {
            pc_net_fail("El anfitrion no mando el pueblo a tiempo.");
        }
        SDL_Delay(10);
    }
    s_booting = 0;
    printf("[Net] town received (%u bytes)\n", s_boot_save_size);
}

void pc_net_init(void) {
    if (g_pc_net_mode == PC_NET_MODE_OFF) {
        return;
    }

#ifdef _WIN32
    {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            pc_net_fail("No se pudo iniciar la red (WSAStartup).");
        }
    }
#endif

    if (g_pc_net_mode == PC_NET_MODE_HOST) {
        pc_net_init_host();
    } else {
        pc_net_init_join();
    }
}

void pc_net_shutdown(void) {
    if (g_pc_net_mode == PC_NET_MODE_OFF) {
        return;
    }
    pc_net_flush();
    if (s_conn != PC_SOCK_INVALID) {
        pc_sock_close(s_conn);
        s_conn = PC_SOCK_INVALID;
    }
    if (s_listen != PC_SOCK_INVALID) {
        pc_sock_close(s_listen);
        s_listen = PC_SOCK_INVALID;
    }
#ifdef _WIN32
    WSACleanup();
#endif
}

const uint8_t* pc_net_get_boot_save(uint32_t* size) {
    *size = s_boot_save_size;
    return s_boot_save;
}

void pc_net_free_boot_save(void) {
    free(s_boot_save);
    s_boot_save = NULL;
    s_boot_save_size = 0;
}
