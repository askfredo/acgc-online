/* pc_net.h - netplay transport: one TCP connection between a host and a guest.
 *
 * The host (--host [port]) owns the town save. The guest (--join ip[:port])
 * connects before the game boots, downloads the host's live town, and plays
 * its own resident in it. Messages are length-prefixed frames; the game-side
 * protocol lives in src/game/m_netplay.c (mNtp_on_message).
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

/* Message types. Payload layouts are defined in m_netplay.c. */
enum {
    PC_NET_MSG_HELLO = 1,   /* guest -> host: protocol version */
    PC_NET_MSG_SAVE,        /* host -> guest: whole town GCI */
    PC_NET_MSG_STATE,       /* both: player position/animation/look (PcNetPlayerState) */
    PC_NET_MSG_FG_FULL_REQ, /* guest -> host: send me your town items */
    PC_NET_MSG_FG_FULL,     /* host -> guest: all town items + buried flags */
    PC_NET_MSG_FG_SET,      /* both: one town item changed */
    PC_NET_MSG_DEPOSIT_SET, /* both: one row of buried-item flags changed */
    PC_NET_MSG_PLAYER,      /* guest -> host: guest resident's data on save */
    PC_NET_MSG_PLAYER_ACK,  /* host -> guest: player data stored */
    PC_NET_MSG_TREE_SHAKE,  /* both: a tree was shaken (visual only) */
    PC_NET_MSG_DOOR,        /* both: a structure's door animation frame (visual only) */
    PC_NET_MSG_EMOTE,       /* both: the player made a gesture (emote index) */
};

#define PC_NET_PROTOCOL_VERSION 5

typedef struct {
    int32_t scene_no;
    int32_t field_id;
    float pos[3];
    int16_t angle_y;
    int16_t anim_idx;
    float anim_frame;
    uint8_t gender;
    uint8_t face;
    uint8_t eye_tex_idx;
    uint8_t mouth_tex_idx;
    uint16_t cloth_idx;
    uint8_t player_no;
    int8_t part_table_idx; /* upper/lower body split between the two animations */
    int16_t anim1_idx;
    int16_t pad;
    float anim1_frame;
    /* held tool: skeleton/animation are static data, sent as offsets in the executable */
    int8_t item_main;
    int8_t pad2;
    int16_t item_shape;
    uint32_t item_skel_ofs;
    uint32_t item_anim_ofs;
    float item_frame;
    float item_scale;
    int16_t net_angle[3];
    int16_t pad3;
} PcNetPlayerState;

#define PC_NET_NO_OFS 0xFFFFFFFFu

extern int g_pc_net_mode;
extern char g_pc_net_addr[64];
extern int g_pc_net_port;

/* Host: start listening. Guest: connect and download the town (blocks until done or fails). */
void pc_net_init(void);
void pc_net_shutdown(void);
/* Accept/read/write; dispatches complete messages to mNtp_on_message. Call once per frame. */
void pc_net_poll(void);
int pc_net_connected(void);
int pc_net_send(int type, const void* data, uint32_t len);
uint32_t pc_net_ticks_ms(void);
void pc_net_sleep_ms(int ms);

/* Guest: the town save received during pc_net_init. */
const uint8_t* pc_net_get_boot_save(uint32_t* size);
void pc_net_free_boot_save(void);

/* Implemented by the game side (m_netplay.c). */
void mNtp_on_message(int type, const void* data, uint32_t len);
void mNtp_on_connect(void);
void mNtp_on_disconnect(void);
void mNtp_tick(void);

#endif /* PC_NET_H */
