/* pc_net.c - minimal UDP netplay transport (see pc_net.h) */
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
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int pc_sock_t;
#define PC_SOCK_INVALID (-1)
#define pc_sock_close close
#endif

#define PC_NET_MAGIC 0x504D4341u /* "ACMP" */
#define PC_NET_VERSION 1
#define PC_NET_TIMEOUT_MS 2000

int g_pc_net_mode = PC_NET_MODE_OFF;
char g_pc_net_addr[64] = "";
int g_pc_net_port = PC_NET_DEFAULT_PORT;

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t pad;
    uint32_t seq;
    PcNetPlayerState state;
} PcNetPacket;

static pc_sock_t s_sock = PC_SOCK_INVALID;
static struct sockaddr_in s_peer;
static int s_have_peer = 0;
static uint32_t s_send_seq = 0;
static uint32_t s_recv_seq = 0;
static PcNetPlayerState s_remote;
static int s_have_remote = 0;
static uint32_t s_last_recv_ms = 0;

static uint32_t pc_net_now_ms(void) {
#ifdef _WIN32
    return GetTickCount();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000u + ts.tv_nsec / 1000000u);
#endif
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

void pc_net_init(void) {
    struct sockaddr_in bind_addr;

    if (g_pc_net_mode == PC_NET_MODE_OFF) {
        return;
    }

#ifdef _WIN32
    {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            fprintf(stderr, "[Net] WSAStartup failed\n");
            g_pc_net_mode = PC_NET_MODE_OFF;
            return;
        }
    }
#endif

    s_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s_sock == PC_SOCK_INVALID || !pc_net_set_nonblocking(s_sock)) {
        fprintf(stderr, "[Net] could not create UDP socket\n");
        g_pc_net_mode = PC_NET_MODE_OFF;
        return;
    }

    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    /* Host listens on the known port; joiner takes any free port. */
    bind_addr.sin_port = htons(g_pc_net_mode == PC_NET_MODE_HOST ? (uint16_t)g_pc_net_port : 0);
    if (bind(s_sock, (struct sockaddr*)&bind_addr, sizeof(bind_addr)) != 0) {
        fprintf(stderr, "[Net] could not bind UDP port %d\n", g_pc_net_port);
        pc_sock_close(s_sock);
        s_sock = PC_SOCK_INVALID;
        g_pc_net_mode = PC_NET_MODE_OFF;
        return;
    }

    if (g_pc_net_mode == PC_NET_MODE_JOIN) {
        struct addrinfo hints;
        struct addrinfo* res = NULL;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;
        if (getaddrinfo(g_pc_net_addr, NULL, &hints, &res) != 0 || res == NULL) {
            fprintf(stderr, "[Net] could not resolve host '%s'\n", g_pc_net_addr);
            pc_sock_close(s_sock);
            s_sock = PC_SOCK_INVALID;
            g_pc_net_mode = PC_NET_MODE_OFF;
            return;
        }
        memcpy(&s_peer, res->ai_addr, sizeof(s_peer));
        s_peer.sin_port = htons((uint16_t)g_pc_net_port);
        freeaddrinfo(res);
        s_have_peer = 1;
        printf("[Net] joining %s:%d\n", g_pc_net_addr, g_pc_net_port);
    } else {
        printf("[Net] hosting on UDP port %d\n", g_pc_net_port);
    }
}

void pc_net_shutdown(void) {
    if (s_sock != PC_SOCK_INVALID) {
        pc_sock_close(s_sock);
        s_sock = PC_SOCK_INVALID;
    }
#ifdef _WIN32
    if (g_pc_net_mode != PC_NET_MODE_OFF) {
        WSACleanup();
    }
#endif
}

static void pc_net_receive(void) {
    PcNetPacket pkt;
    struct sockaddr_in from;

    for (;;) {
        socklen_t from_len = sizeof(from);
        int n = (int)recvfrom(s_sock, (char*)&pkt, sizeof(pkt), 0, (struct sockaddr*)&from, &from_len);

        if (n < 0) {
            break; /* EWOULDBLOCK: queue drained */
        }
        if (n != (int)sizeof(pkt) || pkt.magic != PC_NET_MAGIC || pkt.version != PC_NET_VERSION) {
            continue;
        }

        /* Host learns (or re-learns) the peer address from incoming packets. */
        if (g_pc_net_mode == PC_NET_MODE_HOST &&
            (!s_have_peer || s_peer.sin_addr.s_addr != from.sin_addr.s_addr || s_peer.sin_port != from.sin_port)) {
            s_peer = from;
            s_have_peer = 1;
            s_recv_seq = 0;
            printf("[Net] peer connected from %s:%d\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port));
        }

        /* Drop out-of-order packets (signed diff handles wraparound). */
        if (s_have_remote && (int32_t)(pkt.seq - s_recv_seq) <= 0) {
            continue;
        }
        s_recv_seq = pkt.seq;
        s_remote = pkt.state;
        s_have_remote = 1;
        s_last_recv_ms = pc_net_now_ms();
    }
}

void pc_net_update(const PcNetPlayerState* local) {
    if (s_sock == PC_SOCK_INVALID) {
        return;
    }

    pc_net_receive();

    if (s_have_peer && local != NULL) {
        PcNetPacket pkt;

        memset(&pkt, 0, sizeof(pkt));
        pkt.magic = PC_NET_MAGIC;
        pkt.version = PC_NET_VERSION;
        pkt.seq = ++s_send_seq;
        pkt.state = *local;
        sendto(s_sock, (const char*)&pkt, sizeof(pkt), 0, (struct sockaddr*)&s_peer, sizeof(s_peer));
    }
}

int pc_net_get_remote(PcNetPlayerState* out) {
    if (!s_have_remote || pc_net_now_ms() - s_last_recv_ms > PC_NET_TIMEOUT_MS) {
        return 0;
    }
    *out = s_remote;
    return 1;
}
