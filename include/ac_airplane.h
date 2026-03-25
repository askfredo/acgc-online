#ifndef AC_AIRPLANE_H
#define AC_AIRPLANE_H

#include "types.h"
#include "m_actor.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
  aAp_STATUS_FREE_FLY_MOVE,
  aAp_STATUS_FALL_FLY_MOVE,
  aAp_STATUS_FALL_FLY_MOVE2,
  aAp_STATUS_SOMER_FLY_MOVE,
  aAp_STATUS_START_FLY_MOVE,
  aAp_STATUS_STOP_FLY_MOVE,
  aAp_STATUS_PLAYER_CATCH,

  aAp_STATUS_NUM
};

enum {
  aAp_TILT_DOWN,
  aAp_TILT_UP,

  aAp_TILT_NUM
};

typedef struct airplane_s {
  ACTOR actor_class;

  s16 status;
  s16 tilt_status;
  f32 speed;
  f32 y_speed;
  f32 rotY_goal;
  f32 rotY_min;

  f32 rotY;
  f32 rotX;
  f32 rotZ;

  int joystick_y;
  int joystick_x;

  s16 ground_timer;

  int wind_frame;
  int wind_change_frame;
  xyz_t* wind;

  u8 _1AC[28]; // unused
} AIRPLANE_ACTOR;

extern ACTOR_PROFILE Airplane_Profile;

#if VERSION >= VER_DELUXE
extern int aAP_PlayerStateOK(GAME* game);
extern s16 aAP_Get_flow_angle(ACTOR* actorx);
extern void aAP_effect_hamon(ACTOR* actorx, GAME* game, s16 arg);
extern void aAp_ReleasePlayerLock(ACTOR* actorx, GAME* game);
#endif

#ifdef __cplusplus
}
#endif

#endif
