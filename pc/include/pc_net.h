/* pc_net.h - minimal UDP netplay: two game instances exchange player state.
 *
 * One instance runs with --host [port], the other with --join ip[:port].
 * Each frame the local player state is sent to the peer; the most recent state
 * received from the peer is drawn as a "ghost" player (see m_player_ghost.c_inc).
 *
 * Kept free of SDL/decomp headers so it can be included from game code. */
#ifndef PC_NET_H
#define PC_NET_H

#include <stdint.h>

#define PC_NET_DEFAULT_PORT 7777

enum {
    PC_NET_MODE_OFF = 0,
    PC_NET_MODE_HOST,
    PC_NET_MODE_JOIN,
};

typedef struct {
    int32_t scene_no;
    float pos[3];
    int16_t angle_y;
    int16_t anim_idx;
    float anim_frame;
    uint8_t gender;
} PcNetPlayerState;

extern int g_pc_net_mode;
extern char g_pc_net_addr[64];
extern int g_pc_net_port;

void pc_net_init(void);
void pc_net_shutdown(void);
/* Send the local state and drain incoming packets. Call once per game frame. */
void pc_net_update(const PcNetPlayerState* local);
/* Returns 1 and fills *out if a recent remote state is available. */
int pc_net_get_remote(PcNetPlayerState* out);

#endif /* PC_NET_H */
