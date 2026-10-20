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
#include "m_player_lib.h"
#include "ac_structure.h"
#include "pc_net.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define mNtp_FG_CELLS (FG_BLOCK_Z_NUM * FG_BLOCK_X_NUM * UT_Z_NUM * UT_X_NUM)
#define mNtp_DEPOSIT_ROWS (FG_BLOCK_X_NUM * FG_BLOCK_Z_NUM * UT_Z_NUM)
#define mNtp_REMOTE_TIMEOUT_FRAMES 120
#define mNtp_TREE_QUEUE 8
#define mNtp_DOOR_TIMEOUT_FRAMES 10
#define mNtp_DOOR_EXIT_RADIUS 200.0f

extern int pc_save_serialize(u8** out, u32* out_size);
extern unsigned int pc_image_base;
extern unsigned int pc_image_end;
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

typedef struct {
    u16 item;
    s16 ut_x;
    s16 ut_z;
    u8 little;
    u8 pad;
} mNtp_tree_shake_c;

/* A structure's door animation, sent every frame while it plays. Structure animations
 * are static data in the executable, so they are identified by their offset in it. */
typedef struct {
    u16 npc_id; /* structure item name */
    u16 pad;
    s32 scene_no;
    s32 field_id;
    f32 home_x;
    f32 home_z;
    u32 anim_ofs;
    f32 start_frame;
    f32 end_frame;
    f32 current_frame;
    f32 speed;
    s32 mode;
} mNtp_door_c;

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

    mNtp_tree_shake_c tree_queue[mNtp_TREE_QUEUE];
    int tree_count;

    mNtp_door_c remote_door; /* latest door frame from the peer */
    u32 remote_door_frame;
    int have_remote_door;
    STRUCTURE_ACTOR* door_actor; /* local structure currently driven by the peer */
    cKF_SkeletonInfo_R_c door_saved_keyframe;
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
    l_ntp.tree_count = 0;
    l_ntp.have_remote_door = FALSE; /* the door is released on the next frame's timeout check */
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
        case PC_NET_MSG_TREE_SHAKE:
            if (len == sizeof(mNtp_tree_shake_c) && l_ntp.tree_count < mNtp_TREE_QUEUE) {
                memcpy(&l_ntp.tree_queue[l_ntp.tree_count++], data, sizeof(mNtp_tree_shake_c));
            }
            break;
        case PC_NET_MSG_DOOR:
            if (len == sizeof(mNtp_door_c)) {
                memcpy(&l_ntp.remote_door, data, sizeof(mNtp_door_c));
                l_ntp.remote_door_frame = l_ntp.frame;
                l_ntp.have_remote_door = TRUE;
            }
            break;
        case PC_NET_MSG_PLAYER_ACK:
            if (len == sizeof(int)) {
                l_ntp.player_ack = *(const int*)data;
            }
            break;
    }
}

/* --- trees --- */

void mNtp_send_tree_shake(u16 item, int ut_x, int ut_z, int little) {
    mNtp_tree_shake_c m;

    if (g_pc_net_mode == PC_NET_MODE_OFF || !l_ntp.synced) {
        return;
    }
    m.item = item;
    m.ut_x = ut_x;
    m.ut_z = ut_z;
    m.little = little != FALSE;
    m.pad = 0;
    pc_net_send(PC_NET_MSG_TREE_SHAKE, &m, sizeof(m));
}

int mNtp_pop_tree_shake(u16* item, int* ut_x, int* ut_z, int* little) {
    mNtp_tree_shake_c* m;

    if (l_ntp.tree_count <= 0) {
        return FALSE;
    }
    m = &l_ntp.tree_queue[0];
    *item = m->item;
    *ut_x = m->ut_x;
    *ut_z = m->ut_z;
    *little = m->little;
    l_ntp.tree_count--;
    memmove(&l_ntp.tree_queue[0], &l_ntp.tree_queue[1], l_ntp.tree_count * sizeof(mNtp_tree_shake_c));
    return TRUE;
}

/* --- doors --- */

static int mNtp_is_structure(ACTOR* actor) {
    return actor->part == ACTOR_PART_ITEM && ITEM_NAME_GET_TYPE(actor->npc_id) == NAME_TYPE_STRUCT;
}

static STRUCTURE_ACTOR* mNtp_find_structure(GAME_PLAY* play, u16 npc_id, f32 x, f32 z) {
    ACTOR* actor;

    for (actor = play->actor_info.list[ACTOR_PART_ITEM].actor; actor != NULL; actor = actor->next_actor) {
        if (mNtp_is_structure(actor) && actor->npc_id == npc_id && ABS(actor->home.position.x - x) < 1.0f &&
            ABS(actor->home.position.z - z) < 1.0f) {
            return (STRUCTURE_ACTOR*)actor;
        }
    }
    return NULL;
}

/* The structure whose door the local player is using: the door label while entering,
 * the nearest structure while walking out of one. */
static STRUCTURE_ACTOR* mNtp_local_door_actor(GAME_PLAY* play) {
    PLAYER_ACTOR* player = GET_PLAYER_ACTOR(play);
    ACTOR* label = (ACTOR*)player->get_door_label_proc((GAME*)play);
    ACTOR* actor;
    STRUCTURE_ACTOR* best = NULL;
    f32 best_dist = mNtp_DOOR_EXIT_RADIUS;

    if (label != NULL) {
        return mNtp_is_structure(label) ? (STRUCTURE_ACTOR*)label : NULL;
    }
    if (!mPlib_check_player_outdoor_start((GAME*)play)) {
        return NULL;
    }

    for (actor = play->actor_info.list[ACTOR_PART_ITEM].actor; actor != NULL; actor = actor->next_actor) {
        if (mNtp_is_structure(actor)) {
            f32 dist = search_position_distanceXZ(&actor->world.position, &player->actor_class.world.position);

            if (dist < best_dist) {
                best_dist = dist;
                best = (STRUCTURE_ACTOR*)actor;
            }
        }
    }
    return best;
}

static void mNtp_send_door(GAME_PLAY* play) {
    STRUCTURE_ACTOR* str = mNtp_local_door_actor(play);
    cKF_SkeletonInfo_R_c* kf;
    mNtp_door_c m;

    if (str == NULL || str->keyframe.animation == NULL) {
        return;
    }
    kf = &str->keyframe;
    m.npc_id = str->actor_class.npc_id;
    m.pad = 0;
    m.scene_no = Save_Get(scene_no);
    m.field_id = mFI_GetFieldId();
    m.home_x = str->actor_class.home.position.x;
    m.home_z = str->actor_class.home.position.z;
    m.anim_ofs = (u32)kf->animation - pc_image_base;
    m.start_frame = kf->frame_control.start_frame;
    m.end_frame = kf->frame_control.end_frame;
    m.current_frame = kf->frame_control.current_frame;
    m.speed = kf->frame_control.speed;
    m.mode = kf->frame_control.mode;
    pc_net_send(PC_NET_MSG_DOOR, &m, sizeof(m));
}

extern void aMHS_PC_set_door_open(ACTOR* actorx, int open);

static void mNtp_set_door_open(STRUCTURE_ACTOR* str, int open) {
    if (str->actor_class.id == mAc_PROFILE_MYHOUSE) {
        aMHS_PC_set_door_open((ACTOR*)str, open);
    }
}

static void mNtp_pose_structure(STRUCTURE_ACTOR* str) {
    f32 frame = str->keyframe.frame_control.current_frame;

    cKF_SkeletonInfo_R_play(&str->keyframe);
    str->keyframe.frame_control.current_frame = frame;
}

static void mNtp_release_door(GAME_PLAY* play) {
    STRUCTURE_ACTOR* str = l_ntp.door_actor;

    l_ntp.door_actor = NULL;
    /* Only touch it if it is still loaded (it may have unloaded with its block). */
    if (str != NULL && mNtp_find_structure(play, l_ntp.remote_door.npc_id, l_ntp.remote_door.home_x,
                                           l_ntp.remote_door.home_z) == str) {
        str->keyframe = l_ntp.door_saved_keyframe;
        mNtp_pose_structure(str);
        mNtp_set_door_open(str, FALSE);
    }
}

static void mNtp_apply_remote_door(GAME_PLAY* play) {
    mNtp_door_c* m = &l_ntp.remote_door;
    STRUCTURE_ACTOR* str;
    cKF_FrameControl_c* fc;

    if (!l_ntp.have_remote_door) {
        if (l_ntp.door_actor != NULL) {
            mNtp_release_door(play);
        }
        return;
    }
    if (l_ntp.frame - l_ntp.remote_door_frame > mNtp_DOOR_TIMEOUT_FRAMES || m->scene_no != Save_Get(scene_no) ||
        m->field_id != mFI_GetFieldId() || m->anim_ofs >= pc_image_end - pc_image_base) {
        l_ntp.have_remote_door = FALSE;
        mNtp_release_door(play);
        return;
    }

    str = mNtp_find_structure(play, m->npc_id, m->home_x, m->home_z);
    if (str == NULL) {
        return;
    }
    if (l_ntp.door_actor != str) {
        mNtp_release_door(play);
        l_ntp.door_actor = str;
        l_ntp.door_saved_keyframe = str->keyframe;
    }

    fc = &str->keyframe.frame_control;
    str->keyframe.animation = (cKF_Animation_R_c*)(pc_image_base + m->anim_ofs);
    fc->start_frame = m->start_frame;
    fc->end_frame = m->end_frame;
    fc->max_frames = str->keyframe.animation->frames;
    fc->speed = m->speed; /* lets the structure's own door sounds fire on the right frames */
    fc->mode = m->mode;
    fc->current_frame = m->current_frame;
    mNtp_pose_structure(str);
    mNtp_set_door_open(str, TRUE);
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
        mNtp_send_door((GAME_PLAY*)game);
    }
    mNtp_apply_remote_door((GAME_PLAY*)game);
}

int mNtp_get_remote(PcNetPlayerState* out) {
    if (mEv_IsTitleDemo() || !l_ntp.have_remote || !pc_net_connected() || l_ntp.frame - l_ntp.remote_frame > mNtp_REMOTE_TIMEOUT_FRAMES) {
        return FALSE;
    }
    *out = l_ntp.remote;
    return TRUE;
}

#endif /* TARGET_PC */
