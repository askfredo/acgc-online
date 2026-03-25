#include "ac_ev_dokutu.h"

#if VERSION >= VER_DELUXE

#include "m_common_data.h"
#include "m_player_lib.h"
#include "m_field_info.h"
#include "m_private.h"
#include "ac_monument.h"
#include "m_msg.h"
#include "m_debug_display.h"
#include "libc64/math64.h"
#include "sys_math.h"

#include "../src/actor/npc/event/ac_ev_dokutu_talk.c_inc"

static void aEVD_actor_ct(ACTOR* actorx, GAME* game);
static void aEVD_actor_dt(ACTOR* actorx, GAME* game);
static void aEVD_actor_init(ACTOR* actorx, GAME* game);
static void aEVD_actor_save(ACTOR* actorx, GAME* game);
static void aEVD_actor_move(ACTOR* actorx, GAME* game);
static void aEVD_actor_draw(ACTOR* actorx, GAME* game);

ACTOR_PROFILE Ev_Dokutu_Profile = {
  mAc_PROFILE_EV_DOKUTU,
  ACTOR_PART_NPC,
  0,
  SP_NPC_EV_DOKUTU,
  ACTOR_OBJ_BANK_KEEP,
  sizeof(EVENT_DOKUTU_ACTOR),
  &aEVD_actor_ct,
  &aEVD_actor_dt,
  &aEVD_actor_init,
  mActor_NONE_PROC1,
  &aEVD_actor_save
};

static aNPC_ct_data_c ct_data = {
  &aEVD_actor_move,
  &aEVD_actor_draw,
  3,
  &aEVD_talk_request,
  &aEVD_talk_init,
  &aEVD_talk_end_chk,
  0
};

static void aEVD_actor_ct(ACTOR* actorx, GAME* game) {
  EVENT_DOKUTU_ACTOR* dokutu = (EVENT_DOKUTU_ACTOR*)actorx;

  if ((*Common_Get(clip).npc_clip->birth_check_proc)(actorx, game) == TRUE) {
    (*Common_Get(clip).npc_clip->ct_proc)(actorx, game, &ct_data);

    /* Deluxe: initialize follow/preview state */
    dokutu->follow_state = aEVD_FOLLOW_IDLE;
    dokutu->talk_state = aEVD_TALK_GREETING;
    dokutu->preview_item = EMPTY_NO;
    dokutu->preview_pos.x = 0.0f;
    dokutu->preview_pos.y = 0.0f;
    dokutu->preview_pos.z = 0.0f;
    dokutu->preview_active = FALSE;
    dokutu->follow_timer = 0;
    dokutu->backup_timer = 0;
    dokutu->selected_art = -1;
    dokutu->request_action = 0;
    dokutu->request_priority = 0;
    dokutu->request_anim = -1;
    dokutu->request_frame = 0;
  }
}

static void aEVD_actor_save(ACTOR* actorx, GAME* game) {
  mNpc_RenewalSetNpc(actorx);
}

static void aEVD_actor_dt(ACTOR* actorx, GAME* game) {
  (*Common_Get(clip).npc_clip->dt_proc)(actorx, game);
}

static void aEVD_actor_init(ACTOR* actorx, GAME* game) {
  (*Common_Get(clip).npc_clip->init_proc)(actorx, game);
}

static void aEVD_actor_move(ACTOR* actorx, GAME* game) {
  EVENT_DOKUTU_ACTOR* dokutu = (EVENT_DOKUTU_ACTOR*)actorx;

  /* Standard NPC move */
  (*Common_Get(clip).npc_clip->move_proc)(actorx, game);

  /* Deluxe: follow behavior */
  switch (dokutu->follow_state) {
    case aEVD_FOLLOW_ACTIVE:
      aEVD_act_follow(dokutu, game);
      break;
    case aEVD_FOLLOW_BACKUP:
      aEVD_backup_from_player(dokutu, GET_PLAYER_ACTOR_GAME_ACTOR(game));
      break;
    case aEVD_FOLLOW_TELEPORT:
      aEVD_teleport_to_player(actorx, GET_PLAYER_ACTOR_GAME_ACTOR(game));
      dokutu->follow_state = aEVD_FOLLOW_ACTIVE;
      break;
    default:
      break;
  }
}

static void aEVD_actor_draw(ACTOR* actorx, GAME* game) {
  EVENT_DOKUTU_ACTOR* dokutu = (EVENT_DOKUTU_ACTOR*)actorx;

  (*Common_Get(clip).npc_clip->draw_proc)(actorx, game);

  /* Deluxe: draw monument placement outline if active */
  if (dokutu->preview_active) {
    aEVD_draw_monument_outline(dokutu, game);
  }
}

/* Deluxe: schedule proc (no-op) */
static void aEVD_schedule_proc(NPC_ACTOR* actor, GAME_PLAY* play, int param) {
}

#endif /* VERSION >= VER_DELUXE */
