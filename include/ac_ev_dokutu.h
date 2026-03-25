#ifndef AC_EV_DOKUTU_H
#define AC_EV_DOKUTU_H

#include "types.h"

#if VERSION >= VER_DELUXE

#include "m_actor.h"
#include "ac_npc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Deluxe: dokutu follow states */
enum {
  aEVD_FOLLOW_IDLE,
  aEVD_FOLLOW_ACTIVE,
  aEVD_FOLLOW_BACKUP,
  aEVD_FOLLOW_TELEPORT,

  aEVD_FOLLOW_NUM
};

/* Deluxe: dokutu talk states */
enum {
  aEVD_TALK_GREETING,
  aEVD_TALK_SELECT_MODE,
  aEVD_TALK_SET_PREVIEW,
  aEVD_TALK_SHOW_PREVIEW,
  aEVD_TALK_WAIT_PREVIEW,
  aEVD_TALK_FINISH_PREVIEW,
  aEVD_TALK_CANT_AFFORD,
  aEVD_TALK_BELLS_HANDOVER,
  aEVD_TALK_PUTAWAY_BELLS,
  aEVD_TALK_GOODBYE,
  /* Deluxe: monument states */
  aEVD_TALK_SELECT_MONUMENT,
  aEVD_TALK_REMOVE_MONUMENT_TALK,
  aEVD_TALK_REMOVE_MONUMENT,

  aEVD_TALK_NUM
};

typedef struct event_dokutu_actor_s {
  NPC_ACTOR npc_class;
  int follow_state;           /* Deluxe: aEVD_FOLLOW_* */
  int talk_state;             /* Deluxe: aEVD_TALK_* */
  mActor_name_t preview_item; /* Deluxe: item being previewed */
  xyz_t preview_pos;          /* Deluxe: preview world position */
  int preview_active;         /* Deluxe: preview displayed flag */
  s16 follow_timer;           /* Deluxe: follow frame counter */
  s16 backup_timer;           /* Deluxe: backup frame counter */
  s16 selected_art;           /* Deluxe: selected art index */
  u8 request_action;          /* Deluxe: queued action type */
  u8 request_priority;        /* Deluxe: action priority */
  s16 request_anim;           /* Deluxe: queued animation */
  s16 request_frame;          /* Deluxe: animation start frame */
} EVENT_DOKUTU_ACTOR;

extern ACTOR_PROFILE Ev_Dokutu_Profile;

#ifdef __cplusplus
}
#endif

#endif /* VERSION >= VER_DELUXE */

#endif

