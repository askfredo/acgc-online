/* m_netplay.c - PC netplay: two residents living in the same town.
 *
 * The host's save is the town. When the guest joins it downloads the host's
 * live save, plays its own resident in it, and:
 *   - both sides send their player state every frame (drawn as a ghost on the other side),
 *   - town items (fg) and buried flags are diffed every frame and changes are sent,
 *     with the host as the authority on conflicts,
 *   - when the guest saves, its resident's data is sent to the host and merged into
 *     the host's save (persisted the next time the host saves).
 *
 * Transport is pc_net (TCP). Nothing here runs unless --host/--join was given. */
#ifdef TARGET_PC

#include "m_common_data.h"
#include "m_field_info.h"
#include "m_field_make.h"
#include "m_house.h"
#include "m_card.h"
#include "m_private.h"
#include "m_event.h"
#include "m_play.h"
#include "m_actor.h"
#include "pc_net.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define mNtp_FG_CELLS (FG_BLOCK_Z_NUM * FG_BLOCK_X_NUM * UT_Z_NUM * UT_X_NUM)
#define mNtp_DEPOSIT_ROWS (FG_BLOCK_X_NUM * FG_BLOCK_Z_NUM * UT_Z_NUM)
#define mNtp_REMOTE_TIMEOUT_FRAMES 120

extern int pc_save_serialize(u8** out, u32* out_size);
extern void* pc_save_get_aram_block(int idx);

typedef struct {
    u16 cell; /* index into the flattened Save fg array */
    u16 old_item;
    u16 new_item;
    u8 force; /* host correction: apply regardless of the current value */
    u8 pad;
} mNtp_fg_set_c;

typedef struct {
    u16 row;
    u16 old_bits;
    u16 new_bits;
    u8 force;
    u8 pad;
} mNtp_deposit_set_c;

typedef struct {
    mFM_fg_c fg[FG_BLOCK_Z_NUM][FG_BLOCK_X_NUM];
    u16 deposit[mNtp_DEPOSIT_ROWS];
} mNtp_fg_full_c;

typedef struct {
    u8 player_no;
    u8 home_no;
    u8 keep_house_size;
    u8 pad;
    Private_c private_data;
    mHm_hs_c home;
    mPr_mother_mail_info_c mother_mail;
    mDi_entry_c diary[mCD_KEEP_DIARY_ENTRY_COUNT];
} mNtp_player_c;

enum {
    mNtp_PLAYER_ACK_OK,
    mNtp_PLAYER_ACK_SAME_PLAYER,
    mNtp_PLAYER_ACK_UNKNOWN_PLAYER,
};

extern int pc_save_loaded;
extern u32 pc_frame_counter;

static struct {
    int save_requested; /* host: guest asked for the town before ours finished loading */
    int synced;        /* fg shadow is valid and diffs flow both ways */
    int sync_requested; /* guest: FG_FULL_REQ sent */
    int have_remote;
    u32 remote_frame;
    u32 frame;
    PcNetPlayerState remote;
    int player_ack; /* guest: -1 waiting, else mNtp_PLAYER_ACK_* */
    u16 fg_shadow[mNtp_FG_CELLS]; /* last synced town items (canonical) */
    u16 deposit_shadow[mNtp_DEPOSIT_ROWS];
    u16 fg_canon[mNtp_FG_CELLS]; /* town items as they would be saved (see mNtp_build_canon) */
    u32 canon_vi_frame;
} l_ntp;

static u16* mNtp_fg_cells(void) {
    return (u16*)Save_GetPointer(fg);
}

static u16* mNtp_deposit_rows(void) {
    return (u16*)Save_GetPointer(deposit);
}

/* Cells hidden under a loaded actor (structures, mailboxes, signs...) hold placeholder
 * values (RSV_NO, DUMMY_*) while the actor is on screen; the real item lives in the
 * actor and is written back when it unloads. These must never cross the network. */
static int mNtp_is_placeholder(u16 item) {
    return item >= DUMMY_START || ITEM_NAME_GET_TYPE(item) == NAME_TYPE_STRUCT;
}

static int mNtp_cell_of(xyz_t pos) {
    u16* p = mFI_GetUnitFG(pos);
    u16* base = mNtp_fg_cells();

    if (p == NULL || p < base || p >= base + mNtp_FG_CELLS) {
        return -1;
    }
    return p - base;
}

/* The town items with every loaded actor's item put back, like restore_fgdata_all
 * does when a scene ends, without touching the live grid. */
static void mNtp_build_canon(GAME_PLAY* play) {
    int part;

    memcpy(l_ntp.fg_canon, mNtp_fg_cells(), sizeof(l_ntp.fg_canon));
    for (part = 0; part < ACTOR_PART_NUM; part++) {
        ACTOR* actor;

        for (actor = play->actor_info.list[part].actor; actor != NULL; actor = actor->next_actor) {
            mActor_name_t name = actor->npc_id;
            int cell;

            if (part != ACTOR_PART_FG && part != ACTOR_PART_ITEM && actor->restore_fg != TRUE) {
                continue;
            }
            if (name == EMPTY_NO || actor->move_actor_list_idx != -1 || actor->block_x < 0 || actor->block_z < 0 ||
                ITEM_NAME_GET_TYPE(name) == NAME_TYPE_ITEM2) {
                continue;
            }
            cell = mNtp_cell_of(actor->home.position);
            if (cell >= 0) {
                l_ntp.fg_canon[cell] = name;
            }
        }
    }
    l_ntp.canon_vi_frame = pc_frame_counter;
}

/* Canonical items for sending outside the player's frame: the cached copy while a
 * play scene is running, else the live grid (no actors hold anything then). */
static const u16* mNtp_canon(void) {
    if (pc_frame_counter - l_ntp.canon_vi_frame <= 2) {
        return l_ntp.fg_canon;
    }
    return mNtp_fg_cells();
}

static void mNtp_take_shadow(const u16* fg) {
    memcpy(l_ntp.fg_shadow, fg, sizeof(l_ntp.fg_shadow));
    memcpy(l_ntp.deposit_shadow, mNtp_deposit_rows(), sizeof(l_ntp.deposit_shadow));
}

static void mNtp_set_cell(int cell, u16 item) {
    u16* live = &mNtp_fg_cells()[cell];

    /* An actor owns this cell right now; it writes its own item back when it unloads. */
    if (mNtp_is_placeholder(*live) || mNtp_is_placeholder(item)) {
        return;
    }
    *live = item;
    l_ntp.fg_shadow[cell] = item;
    mFI_SetFGUpData();
}

static void mNtp_set_deposit(int row, u16 bits) {
    mNtp_deposit_rows()[row] = bits;
    l_ntp.deposit_shadow[row] = bits;
}

/* --- connection events (called from pc_net) --- */

void mNtp_on_connect(void) {
    l_ntp.save_requested = FALSE;
    l_ntp.synced = FALSE;
    l_ntp.have_remote = FALSE;
}

void mNtp_on_disconnect(void) {
    l_ntp.synced = FALSE;
    l_ntp.sync_requested = FALSE;
    l_ntp.have_remote = FALSE;
}

/* --- host side --- */

static void mNtp_host_send_save(void) {
    static u16 live_fg[mNtp_FG_CELLS];
    u8* buf;
    u32 size;
    int ok;

    /* Serialize with the canonical items so the guest's structures spawn normally. */
    memcpy(live_fg, mNtp_fg_cells(), sizeof(live_fg));
    memcpy(mNtp_fg_cells(), mNtp_canon(), sizeof(live_fg));
    ok = pc_save_serialize(&buf, &size);
    memcpy(mNtp_fg_cells(), live_fg, sizeof(live_fg));

    if (ok) {
        pc_net_send(PC_NET_MSG_SAVE, buf, size);
        free(buf);
        printf("[Netplay] sent town to guest (%u bytes)\n", (unsigned)size);
    }
}

static void mNtp_host_send_fg_full(void) {
    static mNtp_fg_full_c full;

    /* Re-baseline so everything the guest receives is reflected in the shadow. */
    mNtp_take_shadow(mNtp_canon());
    memcpy(full.fg, l_ntp.fg_shadow, sizeof(full.fg));
    memcpy(full.deposit, mNtp_deposit_rows(), sizeof(full.deposit));
    pc_net_send(PC_NET_MSG_FG_FULL, &full, sizeof(full));
    l_ntp.synced = TRUE;
    printf("[Netplay] sent town items to guest\n");
}

static void mNtp_host_recv_player(const mNtp_player_c* p) {
    int ack = mNtp_PLAYER_ACK_OK;
    int no = p->player_no;
    Private_c* priv;

    if (no < 0 || no >= PLAYER_NUM || p->home_no >= PLAYER_NUM) {
        return;
    }
    priv = Save_GetPointer(private_data[no]);

    if (no == Common_Get(player_no)) {
        /* Both playing the same resident: never overwrite the host's live player. */
        ack = mNtp_PLAYER_ACK_SAME_PLAYER;
    } else if (!priv->exists || !mPr_CheckCmpPersonalID(&priv->player_ID, (PersonalID_c*)&p->private_data.player_ID)) {
        ack = mNtp_PLAYER_ACK_UNKNOWN_PLAYER;
    } else {
        mCD_keep_diary_c* diary = (mCD_keep_diary_c*)pc_save_get_aram_block(mCD_ARAM_DATA_DIARY);

        *priv = p->private_data;
        Save_Set(homes[p->home_no], p->home);
        Save_Set(keep_house_size[p->home_no], p->keep_house_size);
        Save_Set(mother_mail[no], p->mother_mail);
        if (diary != NULL) {
            memcpy(diary->entries[no], p->diary, sizeof(p->diary));
        }
        printf("[Netplay] stored guest resident %d (saved with the host's next save)\n", no);
    }

    pc_net_send(PC_NET_MSG_PLAYER_ACK, &ack, sizeof(ack));
}

/* --- guest side --- */

static void mNtp_guest_recv_fg_full(const mNtp_fg_full_c* full) {
    const u16* items = (const u16*)full->fg;
    u16* live = mNtp_fg_cells();
    int i;

    for (i = 0; i < mNtp_FG_CELLS; i++) {
        if (!mNtp_is_placeholder(live[i]) && !mNtp_is_placeholder(items[i])) {
            live[i] = items[i];
        }
    }
    memcpy(mNtp_deposit_rows(), full->deposit, sizeof(full->deposit));
    mNtp_take_shadow(items);
    mFI_SetFGUpData();
    l_ntp.synced = TRUE;
    printf("[Netplay] town items synced from host\n");
}

int mNtp_guest_send_player(void) {
    static mNtp_player_c p;
    int no = Common_Get(player_no);
    int home_no;
    mCD_keep_diary_c* diary = (mCD_keep_diary_c*)pc_save_get_aram_block(mCD_ARAM_DATA_DIARY);
    uint32_t start;

    if (!pc_net_connected() || no < 0 || no >= PLAYER_NUM) {
        return FALSE;
    }

    home_no = mHS_get_arrange_idx(no);
    memset(&p, 0, sizeof(p));
    p.player_no = no;
    p.home_no = home_no;
    p.keep_house_size = Save_Get(keep_house_size[home_no]);
    p.private_data = Save_Get(private_data[no]);
    p.home = Save_Get(homes[home_no]);
    p.mother_mail = Save_Get(mother_mail[no]);
    if (diary != NULL) {
        memcpy(p.diary, diary->entries[no], sizeof(p.diary));
    }

    l_ntp.player_ack = -1;
    pc_net_send(PC_NET_MSG_PLAYER, &p, sizeof(p));

    /* The save menu waits on this call; give the host a moment to confirm. */
    start = pc_net_ticks_ms();
    while (l_ntp.player_ack < 0 && pc_net_connected() && pc_net_ticks_ms() - start < 5000) {
        pc_net_poll();
        pc_net_sleep_ms(5);
    }

    if (l_ntp.player_ack != mNtp_PLAYER_ACK_OK) {
        printf("[Netplay] host did not store this resident (ack %d)\n", l_ntp.player_ack);
        return FALSE;
    }
    return TRUE;
}

/* Called by pc_net every frame, including before the game has loaded a save. */
void mNtp_tick(void) {
    if (l_ntp.save_requested && pc_save_loaded) {
        l_ntp.save_requested = FALSE;
        mNtp_host_send_save();
    }
}

/* --- messages --- */

static void mNtp_recv_fg_set(const mNtp_fg_set_c* m) {
    u16 cur;

    if (!l_ntp.synced || m->cell >= mNtp_FG_CELLS) {
        return;
    }
    cur = mNtp_fg_cells()[m->cell];

    if (mNtp_is_placeholder(cur)) {
        return; /* an actor owns the cell here; it keeps its own item */
    }

    if (g_pc_net_mode == PC_NET_MODE_JOIN) {
        /* Host is authoritative. */
        if (cur != m->new_item) {
            mNtp_set_cell(m->cell, m->new_item);
        } else {
            l_ntp.fg_shadow[m->cell] = m->new_item;
        }
    } else if (cur == m->old_item) {
        mNtp_set_cell(m->cell, m->new_item);
    } else if (cur != m->new_item) {
        /* Guest acted on a stale cell (e.g. both grabbed the same item): correct it. */
        mNtp_fg_set_c fix;

        fix.cell = m->cell;
        fix.old_item = m->new_item;
        fix.new_item = cur;
        fix.force = TRUE;
        fix.pad = 0;
        pc_net_send(PC_NET_MSG_FG_SET, &fix, sizeof(fix));
    }
}

static void mNtp_recv_deposit_set(const mNtp_deposit_set_c* m) {
    u16 cur;

    if (!l_ntp.synced || m->row >= mNtp_DEPOSIT_ROWS) {
        return;
    }
    cur = mNtp_deposit_rows()[m->row];

    if (g_pc_net_mode == PC_NET_MODE_JOIN) {
        mNtp_set_deposit(m->row, m->new_bits);
    } else if (cur == m->old_bits) {
        mNtp_set_deposit(m->row, m->new_bits);
    } else if (cur != m->new_bits) {
        /* Merge independent buried spots, both sides keep digging. */
        u16 merged = (cur & ~(m->old_bits ^ m->new_bits)) | (m->new_bits & (m->old_bits ^ m->new_bits));
        mNtp_deposit_set_c fix;

        mNtp_set_deposit(m->row, merged);
        fix.row = m->row;
        fix.old_bits = m->new_bits;
        fix.new_bits = merged;
        fix.force = TRUE;
        fix.pad = 0;
        pc_net_send(PC_NET_MSG_DEPOSIT_SET, &fix, sizeof(fix));
    }
}

void mNtp_on_message(int type, const void* data, uint32_t len) {
    switch (type) {
        case PC_NET_MSG_HELLO:
            if (g_pc_net_mode == PC_NET_MODE_HOST && len == sizeof(u32) &&
                *(const u32*)data == PC_NET_PROTOCOL_VERSION) {
                l_ntp.save_requested = TRUE;
                mNtp_tick();
            }
            break;
        case PC_NET_MSG_STATE:
            if (len == sizeof(PcNetPlayerState)) {
                memcpy(&l_ntp.remote, data, sizeof(PcNetPlayerState));
                l_ntp.have_remote = TRUE;
                l_ntp.remote_frame = l_ntp.frame;
            }
            break;
        case PC_NET_MSG_FG_FULL_REQ:
            if (g_pc_net_mode == PC_NET_MODE_HOST) {
                mNtp_host_send_fg_full();
            }
            break;
        case PC_NET_MSG_FG_FULL:
            if (g_pc_net_mode == PC_NET_MODE_JOIN && len == sizeof(mNtp_fg_full_c)) {
                mNtp_guest_recv_fg_full((const mNtp_fg_full_c*)data);
            }
            break;
        case PC_NET_MSG_FG_SET:
            if (len == sizeof(mNtp_fg_set_c)) {
                mNtp_recv_fg_set((const mNtp_fg_set_c*)data);
            }
            break;
        case PC_NET_MSG_DEPOSIT_SET:
            if (len == sizeof(mNtp_deposit_set_c)) {
                mNtp_recv_deposit_set((const mNtp_deposit_set_c*)data);
            }
            break;
        case PC_NET_MSG_PLAYER:
            if (g_pc_net_mode == PC_NET_MODE_HOST && len == sizeof(mNtp_player_c)) {
                mNtp_host_recv_player((const mNtp_player_c*)data);
            }
            break;
        case PC_NET_MSG_PLAYER_ACK:
            if (len == sizeof(int)) {
                l_ntp.player_ack = *(const int*)data;
            }
            break;
    }
}

/* --- per frame (from the local player's move) --- */

static void mNtp_send_diffs(void) {
    const u16* cells = l_ntp.fg_canon;
    u16* rows = mNtp_deposit_rows();
    int i;

    for (i = 0; i < mNtp_FG_CELLS; i++) {
        if (cells[i] != l_ntp.fg_shadow[i] && !mNtp_is_placeholder(cells[i]) &&
            !mNtp_is_placeholder(l_ntp.fg_shadow[i])) {
            mNtp_fg_set_c m;

            m.cell = i;
            m.old_item = l_ntp.fg_shadow[i];
            m.new_item = cells[i];
            m.force = FALSE;
            m.pad = 0;
            pc_net_send(PC_NET_MSG_FG_SET, &m, sizeof(m));
            l_ntp.fg_shadow[i] = cells[i];
        }
    }

    for (i = 0; i < mNtp_DEPOSIT_ROWS; i++) {
        if (rows[i] != l_ntp.deposit_shadow[i]) {
            mNtp_deposit_set_c m;

            m.row = i;
            m.old_bits = l_ntp.deposit_shadow[i];
            m.new_bits = rows[i];
            m.force = FALSE;
            m.pad = 0;
            pc_net_send(PC_NET_MSG_DEPOSIT_SET, &m, sizeof(m));
            l_ntp.deposit_shadow[i] = rows[i];
        }
    }
}

void mNtp_player_update(GAME* game, const PcNetPlayerState* local) {
    l_ntp.frame++;

    if (g_pc_net_mode != PC_NET_MODE_OFF) {
        mNtp_build_canon((GAME_PLAY*)game);
    }

    /* The title screen demo walks a puppet player around the town: not a real session. */
    if (g_pc_net_mode == PC_NET_MODE_OFF || !pc_net_connected() || mEv_IsTitleDemo()) {
        return;
    }

    pc_net_send(PC_NET_MSG_STATE, local, sizeof(*local));

    if (g_pc_net_mode == PC_NET_MODE_JOIN && !l_ntp.sync_requested) {
        /* First frame in the world: take the host's items as the baseline. */
        pc_net_send(PC_NET_MSG_FG_FULL_REQ, NULL, 0);
        l_ntp.sync_requested = TRUE;
    }

    if (l_ntp.synced) {
        mNtp_send_diffs();
    }
}

int mNtp_get_remote(PcNetPlayerState* out) {
    if (mEv_IsTitleDemo() || !l_ntp.have_remote || !pc_net_connected() || l_ntp.frame - l_ntp.remote_frame > mNtp_REMOTE_TIMEOUT_FRAMES) {
        return FALSE;
    }
    *out = l_ntp.remote;
    return TRUE;
}

#endif /* TARGET_PC */
