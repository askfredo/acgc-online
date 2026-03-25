#include "ac_shrine.h"

#include "m_msg.h"
#include "m_play.h"
#include "sys_matrix.h"

static void aSHR_actor_ct(ACTOR* actorx, GAME* game);
static void aSHR_actor_dt(ACTOR* actorx, GAME* game);
static void aSHR_actor_init(ACTOR* actorx, GAME* game);
static void aSHR_actor_draw(ACTOR* actorx, GAME* game);

#if VERSION >= VER_DELUXE
extern void aSHR_anime_ct(SHRINE_ACTOR* shrine, f32 speed);
extern void aSHR_actor_ct_jp(ACTOR* actorx, GAME* game);
extern void aSHR_actor_draw_jp(ACTOR* actorx, GAME* game);
#endif

ACTOR_PROFILE Shrine_Profile = {
  mAc_PROFILE_SHRINE,
  ACTOR_PART_ITEM,
  ACTOR_STATE_NONE,
  WISHING_WELL,
  ACTOR_OBJ_BANK_KEEP,
  sizeof(SHRINE_ACTOR),
  &aSHR_actor_ct,
  &aSHR_actor_dt,
  &aSHR_actor_init,
  &aSHR_actor_draw,
  NULL
};

static aSHR_Present_c aSHR_present;
static aSHR_Clip_c aSHR_clip;

enum {
  aSHR_ACTION_WAIT,
  aSHR_ACTION_TALK,
  aSHR_ACTION_TALK_GOMEN,
  aSHR_ACTION_MAKE_HEM,
  aSHR_ACTION_TALK_END,

  aSHR_ACTION_NUM
};

enum {
  aSHR_LEAF_TYPE_SUMMER,
  aSHR_LEAF_TYPE_WINTER,
  aSHR_LEAF_TYPE_CHRISTMAS,
  aSHR_LEAF_TYPE_CHERRY,
  
  aSHR_LEAF_TYPE_NUM
};

static u8 aSHR_shadow_vtx_fix_flg_table[16] = {
  TRUE, FALSE, FALSE, TRUE,
  TRUE, TRUE, FALSE, FALSE,
  TRUE, TRUE, FALSE, FALSE,
  TRUE, TRUE, FALSE, FALSE
};

extern Vtx obj_shrine_shadow_v[];
extern Gfx obj_shrine_shadow_model[];

static bIT_ShadowData_c aSHR_shadow_data = {
  16,
  aSHR_shadow_vtx_fix_flg_table,
  60.0f,
  obj_shrine_shadow_v,
  obj_shrine_shadow_model
};

static void aSHR_setup_action(SHRINE_ACTOR*, GAME_PLAY*, int);
static void aSHR_set_bgOffset(ACTOR*, int);
static void aSHR_Present_ct(aSHR_Present_c*);
static int aSHR_ctrl_light();
static void aSHR_init_clip_area();
static void aSHR_free_clip_area();

static int aSHR_getLeaftype(int winter) {
  if (Common_Get(time).term_idx == mTM_TERM_15) {
    return aSHR_LEAF_TYPE_CHRISTMAS;
  }

  if (Common_Get(time).term_idx == mTM_TERM_16) {
    return aSHR_LEAF_TYPE_CHRISTMAS;
  }

  if (Common_Get(time).term_idx == 4) {
    return aSHR_LEAF_TYPE_CHERRY;
  }

  return winter;
}

static void aSHR_actor_ct(ACTOR* actorx, GAME* game) {
  SHRINE_ACTOR* shrine = (SHRINE_ACTOR*)actorx;
  GAME_PLAY* play = (GAME_PLAY*)game;
  int winter;

  shrine->structure_class.season = Common_Get(time).season;
  winter = shrine->structure_class.season == mTM_SEASON_WINTER;

  aSHR_init_clip_area();
  aSHR_Present_ct(&aSHR_present);
  aSHR_set_bgOffset(actorx, 1);
  shrine->structure_class.arg2 = mEv_check_status(mEv_EVENT_NEW_YEARS_DAY, mEv_STATUS_ACTIVE);
  shrine->structure_class.arg1 = aSHR_getLeaftype(winter);
  actorx->world.position.x = actorx->world.position.x + 20.0f;
  actorx->world.position.z = actorx->world.position.z - 19.0f;
  actorx->talk_distance = 100.0f;
  aSHR_setup_action(shrine, play, aSHR_ACTION_WAIT);
  actorx->cull_radius = 900.0f;
  actorx->cull_height = 1400.0f;
  shrine->structure_class.arg0_f = aSHR_ctrl_light() ? 1.0f : 0.0f;
}

static void aSHR_actor_dt(ACTOR* actorx, GAME* game) {
  SHRINE_ACTOR* shrine = (SHRINE_ACTOR*)actorx;

  aSHR_free_clip_area();
  cKF_SkeletonInfo_R_dt(&shrine->structure_class.keyframe);
}

#include "../src/actor/ac_shrine_clip.c_inc"
#include "../src/actor/ac_shrine_move.c_inc"
#include "../src/actor/ac_shrine_draw.c_inc"

#if VERSION >= VER_DELUXE
extern void aSHR_anime_ct(SHRINE_ACTOR* shrine, f32 speed) {
    cKF_SkeletonInfo_R_c* keyframe = &shrine->structure_class.keyframe;

    keyframe->frame_control.speed = speed;
    cKF_SkeletonInfo_R_play(keyframe);
}

extern void aSHR_actor_ct_jp(ACTOR* actorx, GAME* game) {
    SHRINE_ACTOR* shrine = (SHRINE_ACTOR*)actorx;
    GAME_PLAY* play = (GAME_PLAY*)game;
    int winter;

    shrine->structure_class.season = Common_Get(time).season;
    winter = shrine->structure_class.season == mTM_SEASON_WINTER;

    aSHR_init_clip_area();
    aSHR_Present_ct(&aSHR_present);
    aSHR_set_bgOffset(actorx, 1);
    shrine->structure_class.arg2 = mEv_check_status(mEv_EVENT_NEW_YEARS_DAY, mEv_STATUS_ACTIVE);
    shrine->structure_class.arg1 = aSHR_getLeaftype(winter);
    actorx->world.position.x = actorx->world.position.x + 20.0f;
    actorx->world.position.z = actorx->world.position.z - 19.0f;
    actorx->talk_distance = 100.0f;
    aSHR_setup_action(shrine, play, aSHR_ACTION_WAIT);
    actorx->cull_radius = 900.0f;
    actorx->cull_height = 1400.0f;
    shrine->structure_class.arg0_f = aSHR_ctrl_light() ? 1.0f : 0.0f;
}

static int aSHR_actor_draw_before(GAME* game, cKF_SkeletonInfo_R_c* keyframe, int joint_idx, Gfx** joint_shape,
                                  u8* joint_flags, void* arg, s_xyz* joint_rot, xyz_t* joint_pos) {
    SHRINE_ACTOR* shrine = (SHRINE_ACTOR*)arg;
    ACTOR* actorx = &shrine->structure_class.actor_class;

    if (joint_idx == 0) {
        Matrix_translate(actorx->world.position.x, actorx->world.position.y, actorx->world.position.z, MTX_LOAD);
        Matrix_RotateY(actorx->shape_info.rotation.y, MTX_MULT);
        Matrix_scale(actorx->scale.x, actorx->scale.y, actorx->scale.z, MTX_MULT);
    }

    return TRUE;
}

static int aSHR_actor_draw_after(GAME* game, cKF_SkeletonInfo_R_c* keyframe, int joint_idx, Gfx** joint_shape,
                                 u8* joint_flags, void* arg, s_xyz* joint_rot, xyz_t* joint_pos) {
    /* Post-draw callback - no additional processing needed */
    return TRUE;
}

extern void aSHR_actor_draw_jp(ACTOR* actorx, GAME* game) {
    SHRINE_ACTOR* shrine = (SHRINE_ACTOR*)actorx;
    cKF_SkeletonInfo_R_c* keyframe = &shrine->structure_class.keyframe;
    GRAPH* graph = game->graph;
    Mtx* mtx;

    _texture_z_light_fog_prim_npc(graph);

    mtx = GRAPH_ALLOC_TYPE(graph, Mtx, keyframe->skeleton->num_shown_joints);
    if (mtx != NULL) {
        u16* pal = (*Common_Get(clip).structure_clip->get_pal_segment_proc)(aSTR_PAL_SHRINE);

        OPEN_DISP(graph);

        gDPPipeSync(NOW_POLY_OPA_DISP++);
        gDPLoadTLUT_Dolphin(NOW_POLY_OPA_DISP++, 15, 16, 1, pal);

        CLOSE_DISP(graph);

        cKF_Si3_draw_R_SV(game, keyframe, mtx, &aSHR_actor_draw_before, &aSHR_actor_draw_after, shrine);
    }

    (*Common_Get(clip).bg_item_clip->draw_shadow_proc)(game, &aSHR_shadow_data, FALSE);
}
#endif /* VERSION >= VER_DELUXE */
