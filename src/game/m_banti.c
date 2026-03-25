#include "m_banti.h"

#include "m_play.h"
#include "libultra/libultra.h"
#include "m_player_lib.h"
#include "m_demo.h"
#include "sys_matrix.h"
#include "m_font.h"
#include "m_common_data.h"

static Banti_c banti;

static void banti_animation_init_hiduke(Banti_anim_c* anim);
static void banti_animation_init_jikan(Banti_anim_c* anim);

extern cKF_Skeleton_R_c cKF_bs_r_clk_hiniti;
extern cKF_Skeleton_R_c cKF_bs_r_clk_jikan;

extern void banti_ct() {
    bzero(&banti, sizeof(Banti_c));

    banti.addressable_type = 0;
    banti.disabled = 0;
    banti.timer = 0;
    banti.alpha = 0.0f;
    banti.disp_left = 0;
    banti.move_left = 0;
    banti.move_timer = 0.0f;

    cKF_SkeletonInfo_R_ct(&banti.month_anim.keyframe, &cKF_bs_r_clk_hiniti, NULL, banti.month_anim.work,
                          banti.month_anim.morph);
    banti_animation_init_hiduke(&banti.month_anim);
    banti.month_anim.keyframe.frame_control.speed = 0.0f;

    cKF_SkeletonInfo_R_ct(&banti.day_anim.keyframe, &cKF_bs_r_clk_hiniti, NULL, banti.day_anim.work,
                          banti.day_anim.morph);
    banti_animation_init_hiduke(&banti.day_anim);
    banti.day_anim.keyframe.frame_control.speed = 0.0f;

    cKF_SkeletonInfo_R_ct(&banti.hour_upper_anim.keyframe, &cKF_bs_r_clk_jikan, NULL, banti.hour_upper_anim.work,
                          banti.hour_upper_anim.morph);
    banti_animation_init_jikan(&banti.hour_upper_anim);
    banti.hour_upper_anim.keyframe.frame_control.speed = 0.0f;

    cKF_SkeletonInfo_R_ct(&banti.hour_lower_anim.keyframe, &cKF_bs_r_clk_jikan, NULL, banti.hour_lower_anim.work,
                          banti.hour_lower_anim.morph);
    banti_animation_init_jikan(&banti.hour_lower_anim);
    banti.hour_lower_anim.keyframe.frame_control.speed = 0.0f;

    cKF_SkeletonInfo_R_ct(&banti.min_upper_anim.keyframe, &cKF_bs_r_clk_jikan, NULL, banti.min_upper_anim.work,
                          banti.min_upper_anim.morph);
    banti_animation_init_jikan(&banti.min_upper_anim);
    banti.min_upper_anim.keyframe.frame_control.speed = 0.0f;

    cKF_SkeletonInfo_R_ct(&banti.min_lower_anim.keyframe, &cKF_bs_r_clk_jikan, NULL, banti.min_lower_anim.work,
                          banti.min_lower_anim.morph);
    banti_animation_init_jikan(&banti.min_lower_anim);
    banti.min_lower_anim.keyframe.frame_control.speed = 0.0f;

    lbRTC_TimeCopy(&banti.next_time, Common_GetPointer(time.rtc_time));
    lbRTC_TimeCopy(&banti.displayed_time, Common_GetPointer(time.rtc_time));
}

extern void banti_dt() {
    cKF_SkeletonInfo_R_dt(&banti.month_anim.keyframe);
    cKF_SkeletonInfo_R_dt(&banti.day_anim.keyframe);
    cKF_SkeletonInfo_R_dt(&banti.hour_upper_anim.keyframe);
    cKF_SkeletonInfo_R_dt(&banti.hour_lower_anim.keyframe);
    cKF_SkeletonInfo_R_dt(&banti.min_upper_anim.keyframe);
    cKF_SkeletonInfo_R_dt(&banti.min_lower_anim.keyframe);
}

static void banti_chk_disp_left(GAME_PLAY* play) {
    xyz_t player_pos_screen;

    Game_play_Projection_Trans(play, &get_player_actor_withoutCheck(play)->actor_class.world.position,
                               &player_pos_screen);

    if (player_pos_screen.x >= 180.0f && player_pos_screen.y >= 150.0f) {
        if (banti.move_left == FALSE) {
            banti.move_left = TRUE;
            banti.move_timer = 0.0f;
        }

        banti.move_timer += 1.0f;

        if (banti.move_timer > 10.0f) {
            banti.move_timer = 10.0f;

            if (banti.alpha < 0.01f) {
                banti.disp_left = TRUE;
                banti.disabled = FALSE;
            } else if (banti.disp_left == FALSE) {
                banti.disabled = TRUE;
            }
        }
    } else {
        if (banti.move_left == TRUE) {
            banti.move_left = FALSE;
            banti.move_timer = 0.0f;
        }

        banti.move_timer += 1.0f;

        if (banti.move_timer > 10.0f) {
            banti.move_timer = 10.0f;

            if (banti.alpha < 0.01f) {
                banti.disp_left = FALSE;
                banti.disabled = FALSE;
            } else if (banti.disp_left == TRUE) {
                banti.disabled = TRUE;
            }
        }
    }
}

static f32 banti_calc_disp_alpha_rate(GAME_PLAY* play) {
    if (mDemo_CheckDemo() || mEv_IsTitleDemo() || banti.disabled) {
        banti.addressable_type = mPlayer_ADDRESSABLE_FALSE_MOVEMENT;
        add_calc(&banti.alpha, 0.0f, 1.0f - sqrtf(0.8), 0.1f,
                 0.005f); /* quickly fade out on demo screen or title or force disabled */
    } else if (banti.addressable_type == mPlayer_ADDRESSABLE_TRUE && play->submenu.process_status == mSM_PROCESS_WAIT) {
        add_calc(&banti.alpha, 1.0f, 1.0f - sqrtf(0.8), 0.0425f, 0.0005f); /* fade in */
    } else {
        add_calc(&banti.alpha, 0.0f, 1.0f - sqrtf(0.8), 0.0425f, 0.0005f); /* fade out */
    }
}

extern cKF_Animation_R_c cKF_ba_r_clk_hiniti;

static void banti_animation_init_hiduke(Banti_anim_c* anim) {
    cKF_SkeletonInfo_R_init(&anim->keyframe, anim->keyframe.skeleton, &cKF_ba_r_clk_hiniti, 1.0f, 15.0f, 1.0f, 0.5f,
                            0.0f, cKF_FRAMECONTROL_STOP, NULL);
}

extern cKF_Animation_R_c cKF_ba_r_clk_jikan;

static void banti_animation_init_jikan(Banti_anim_c* anim) {
    cKF_SkeletonInfo_R_init(&anim->keyframe, anim->keyframe.skeleton, &cKF_ba_r_clk_jikan, 1.0f, 15.0f, 1.0f, 0.5f,
                            0.0f, cKF_FRAMECONTROL_STOP, NULL);
}

static void banti_anime_proc() {
    if (cKF_SkeletonInfo_R_play(&banti.month_anim.keyframe) == cKF_STATE_STOPPED) {
        banti.month_anim.keyframe.frame_control.current_frame = 1.0f;
        banti.month_anim.keyframe.frame_control.speed = 0.0f;
        banti.month_anim.state = banti_ANIM_STATE_STOPPED;
    }

    if (cKF_SkeletonInfo_R_play(&banti.day_anim.keyframe) == cKF_STATE_STOPPED) {
        banti.day_anim.keyframe.frame_control.current_frame = 1.0f;
        banti.day_anim.keyframe.frame_control.speed = 0.0f;
        banti.day_anim.state = banti_ANIM_STATE_STOPPED;
    }

    if (cKF_SkeletonInfo_R_play(&banti.hour_upper_anim.keyframe) == cKF_STATE_STOPPED) {
        banti.hour_upper_anim.keyframe.frame_control.current_frame = 1.0f;
        banti.hour_upper_anim.keyframe.frame_control.speed = 0.0f;
        banti.hour_upper_anim.state = banti_ANIM_STATE_STOPPED;
    }

    if (cKF_SkeletonInfo_R_play(&banti.hour_lower_anim.keyframe) == cKF_STATE_STOPPED) {
        banti.hour_lower_anim.keyframe.frame_control.current_frame = 1.0f;
        banti.hour_lower_anim.keyframe.frame_control.speed = 0.0f;
        banti.hour_lower_anim.state = banti_ANIM_STATE_STOPPED;
    }

    if (cKF_SkeletonInfo_R_play(&banti.min_upper_anim.keyframe) == cKF_STATE_STOPPED) {
        banti.min_upper_anim.keyframe.frame_control.current_frame = 1.0f;
        banti.min_upper_anim.keyframe.frame_control.speed = 0.0f;
        banti.min_upper_anim.state = banti_ANIM_STATE_STOPPED;
    }

    if (cKF_SkeletonInfo_R_play(&banti.min_lower_anim.keyframe) == cKF_STATE_STOPPED) {
        banti.min_lower_anim.keyframe.frame_control.current_frame = 1.0f;
        banti.min_lower_anim.keyframe.frame_control.speed = 0.0f;
        banti.min_lower_anim.state = banti_ANIM_STATE_STOPPED;
    }
}

static void banti_chk_anime_start() {
    lbRTC_time_c* rtc_time = Common_GetPointer(time.rtc_time);
    u8 banti_hour;
    u8 rtc_hour;
    u8 banti_min;
    u8 rtc_min;

    int disp0;
    int disp1;
    int work0;
    int work1;

    if (banti.month_anim.state == banti_ANIM_STATE_STOPPED &&
        banti.displayed_time.month != Common_GetPointer(time.rtc_time)->month) {
        banti_animation_init_hiduke(&banti.month_anim);
        banti.month_anim.state = banti_ANIM_STATE_PLAYING;
    }

    if (banti.day_anim.state == banti_ANIM_STATE_STOPPED &&
        banti.displayed_time.day != Common_GetPointer(time.rtc_time)->day) {
        banti_animation_init_hiduke(&banti.day_anim);
        banti.day_anim.state = banti_ANIM_STATE_PLAYING;
    }

    if (banti.displayed_time.hour == 0 || banti.displayed_time.hour == 12) {
        work0 = 12;
    } else if (banti.displayed_time.hour > 11) {
        work0 = banti.displayed_time.hour - 12;
    } else {
        work0 = banti.displayed_time.hour;
    }

    if (Common_Get(time.rtc_time.hour) == 0 || Common_Get(time.rtc_time.hour) == 12) {
        work1 = 12;
    } else if (Common_Get(time.rtc_time.hour) > 11) {
        work1 = Common_Get(time.rtc_time.hour) - 12;
    } else {
        work1 = Common_Get(time.rtc_time.hour);
    }

    disp0 = work0 / 10;
    disp1 = work1 / 10;
    if (banti.hour_upper_anim.state == banti_ANIM_STATE_STOPPED && disp0 != disp1) {
        banti_animation_init_jikan(&banti.hour_upper_anim);
        banti.hour_upper_anim.state = banti_ANIM_STATE_PLAYING;
    }

    disp0 = work0 % 10;
    disp1 = work1 % 10;
    if (banti.hour_lower_anim.state == banti_ANIM_STATE_STOPPED && disp0 != disp1) {
        banti_animation_init_jikan(&banti.hour_lower_anim);
        banti.hour_lower_anim.state = banti_ANIM_STATE_PLAYING;
    }

    disp0 = banti.displayed_time.min / 10;
    disp1 = Common_GetPointer(time.rtc_time)->min / 10;
    if (banti.min_upper_anim.state == banti_ANIM_STATE_STOPPED && disp0 != disp1) {
        banti_animation_init_jikan(&banti.min_upper_anim);
        banti.min_upper_anim.state = banti_ANIM_STATE_PLAYING;
    }

    disp0 = banti.displayed_time.min % 10;
    disp1 = Common_GetPointer(time.rtc_time)->min % 10;
    if (banti.min_lower_anim.state == banti_ANIM_STATE_STOPPED && disp0 != disp1) {
        banti_animation_init_jikan(&banti.min_lower_anim);
        banti.min_lower_anim.state = banti_ANIM_STATE_PLAYING;
    }
}

static void banti_time_check() {
    int pre_state;
    int post_state;

    pre_state = banti.month_anim.state | banti.day_anim.state | banti.hour_upper_anim.state |
                banti.hour_lower_anim.state | banti.min_upper_anim.state | banti.min_lower_anim.state;

    if (pre_state == banti_ANIM_STATE_STOPPED) {
        banti_chk_anime_start();
    }

    banti_anime_proc();

    post_state = banti.month_anim.state | banti.day_anim.state | banti.hour_upper_anim.state |
                 banti.hour_lower_anim.state | banti.min_upper_anim.state | banti.min_lower_anim.state;

    if (pre_state != post_state) {
        if (pre_state == banti_ANIM_STATE_STOPPED) {
            lbRTC_TimeCopy(&banti.next_time, Common_GetPointer(time.rtc_time));
        } else {
            lbRTC_TimeCopy(&banti.displayed_time, &banti.next_time);
        }
    }
}

extern void banti_move(GAME_PLAY* play) {
    int addressable_type;
    int update;

    banti_chk_disp_left(play);

    addressable_type = mPlib_Get_address_able_display();

    if (banti.addressable_type != addressable_type) {
        update = FALSE;
        banti.timer++;

        if (addressable_type == mPlayer_ADDRESSABLE_FALSE_READY_NET) {
            if (banti.timer > 50) {
                update = TRUE;
            }
        } else if (banti.addressable_type != mPlayer_ADDRESSABLE_TRUE) {
            if (banti.timer > 30 || addressable_type == mPlayer_ADDRESSABLE_FALSE_TALKING) {
                update = TRUE;
            }
        } else if (banti.timer > 50 || addressable_type == mPlayer_ADDRESSABLE_FALSE_TALKING) {
            update = TRUE;
        }

        if (update == TRUE) {
            banti.timer = 0;
            banti.addressable_type = addressable_type;
        }
    } else {
        banti.timer = 0;
    }

    banti_calc_disp_alpha_rate(play);
    banti_time_check();
}

static int banti_draw_before(GAME* game, cKF_SkeletonInfo_R_c* keyframe, int joint_idx, Gfx** joint_shape,
                             u8* joint_flags, void* arg, s_xyz* joint_rot, xyz_t* joint_pos) {
    Gfx** gfx_pp;
    Mtx* mtx;
    Banti_callback_c* callback_data = (Banti_callback_c*)arg;

    gfx_pp = callback_data->gfx_pp;
    mtx = callback_data->mtx;

    Matrix_softcv3_mult(joint_pos, joint_rot);
    if (*joint_shape != NULL) {
        _Matrix_to_Mtx(mtx);

        gSPMatrix(gfx_pp[0]++, mtx, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

        if (callback_data->state == banti_ANIM_STATE_STOPPED) {
            if (joint_idx != 2) {
                gSPDisplayList(gfx_pp[0]++, *joint_shape);
            }
        } else {
            gSPDisplayList(gfx_pp[0]++, *joint_shape);
        }

        callback_data->mtx++;
    }

    return 0;
}

extern u8 clk_win_suuji1_TA_tex_txt[];
extern u8 clk_win_suuji2_TA_tex_txt[];
extern u8 clk_win_suuji3_TA_tex_txt[];
extern u8 clk_win_suuji4_TA_tex_txt[];
extern u8 clk_win_suuji5_TA_tex_txt[];
extern u8 clk_win_suuji6_TA_tex_txt[];
extern u8 clk_win_suuji7_TA_tex_txt[];
extern u8 clk_win_suuji8_TA_tex_txt[];
extern u8 clk_win_suuji9_TA_tex_txt[];
extern u8 clk_win_suuji10_TA_tex_txt[];
extern u8 clk_win_suuji11_TA_tex_txt[];
extern u8 clk_win_suuji12_TA_tex_txt[];
extern u8 clk_win_suuji13_TA_tex_txt[];
extern u8 clk_win_suuji14_TA_tex_txt[];
extern u8 clk_win_suuji15_TA_tex_txt[];
extern u8 clk_win_suuji16_TA_tex_txt[];
extern u8 clk_win_suuji17_TA_tex_txt[];
extern u8 clk_win_suuji18_TA_tex_txt[];
extern u8 clk_win_suuji19_TA_tex_txt[];
extern u8 clk_win_suuji20_TA_tex_txt[];
extern u8 clk_win_suuji21_TA_tex_txt[];
extern u8 clk_win_suuji22_TA_tex_txt[];
extern u8 clk_win_suuji23_TA_tex_txt[];
extern u8 clk_win_suuji24_TA_tex_txt[];
extern u8 clk_win_suuji25_TA_tex_txt[];
extern u8 clk_win_suuji26_TA_tex_txt[];
extern u8 clk_win_suuji27_TA_tex_txt[];
extern u8 clk_win_suuji28_TA_tex_txt[];
extern u8 clk_win_suuji29_TA_tex_txt[];
extern u8 clk_win_suuji30_TA_tex_txt[];
extern u8 clk_win_suuji31_TA_tex_txt[];

static void banti_draw_hiduke_sub(Gfx** gfx_pp, GAME* game, Banti_anim_c* anim, int tex0_idx, int tex1_idx,
                                  int pos_idx) {
    static u8* hiniti_tex_table[31] = {
        clk_win_suuji1_TA_tex_txt,  clk_win_suuji2_TA_tex_txt,  clk_win_suuji3_TA_tex_txt,  clk_win_suuji4_TA_tex_txt,
        clk_win_suuji5_TA_tex_txt,  clk_win_suuji6_TA_tex_txt,  clk_win_suuji7_TA_tex_txt,  clk_win_suuji8_TA_tex_txt,
        clk_win_suuji9_TA_tex_txt,  clk_win_suuji10_TA_tex_txt, clk_win_suuji11_TA_tex_txt, clk_win_suuji12_TA_tex_txt,
        clk_win_suuji13_TA_tex_txt, clk_win_suuji14_TA_tex_txt, clk_win_suuji15_TA_tex_txt, clk_win_suuji16_TA_tex_txt,
        clk_win_suuji17_TA_tex_txt, clk_win_suuji18_TA_tex_txt, clk_win_suuji19_TA_tex_txt, clk_win_suuji20_TA_tex_txt,
        clk_win_suuji21_TA_tex_txt, clk_win_suuji22_TA_tex_txt, clk_win_suuji23_TA_tex_txt, clk_win_suuji24_TA_tex_txt,
        clk_win_suuji25_TA_tex_txt, clk_win_suuji26_TA_tex_txt, clk_win_suuji27_TA_tex_txt, clk_win_suuji28_TA_tex_txt,
        clk_win_suuji29_TA_tex_txt, clk_win_suuji30_TA_tex_txt, clk_win_suuji31_TA_tex_txt
    };

    static xyz_t set_pos_table[2] = { { 64.0f, -70.0f, 0.0f }, { 94.0f, -70.0f, 0.0f } };

    GRAPH* g = game->graph;
    xyz_t* set_pos = &set_pos_table[pos_idx];
    Mtx* m;
    int joint;
    Banti_callback_c callback_data;

    Matrix_push();
    Matrix_translate(set_pos->x, set_pos->y, set_pos->z, MTX_MULT);

    m = (Mtx*)GRAPH_ALLOC_TYPE(g, Mtx, anim->keyframe.skeleton->num_shown_joints);
    callback_data.mtx = m;

    if (m != NULL) {
        joint = 0;
        callback_data.gfx_pp = gfx_pp;
        callback_data.state = anim->state;

        gSPSegment(gfx_pp[0]++, G_MWO_SEGMENT_8, hiniti_tex_table[tex0_idx]);
        gSPSegment(gfx_pp[0]++, G_MWO_SEGMENT_9, hiniti_tex_table[tex1_idx]);

        cKF_Si3_draw_SV_R_child(game, &anim->keyframe, &joint, &banti_draw_before, NULL, &callback_data, NULL);
    }

    Matrix_pull();
}

static void banti_draw_hiduke(Gfx** gfx_pp, GAME_PLAY* play, int alpha) {
    u8 a = alpha;

    gDPSetPrimColor(gfx_pp[0]++, 0, a, 235, 255, 120, a);
    gDPSetEnvColor(gfx_pp[0]++, 80, 40, 40, a);

    banti_draw_hiduke_sub(gfx_pp, (GAME*)play, &banti.month_anim, banti.displayed_time.month - 1,
                          banti.next_time.month - 1, 0);
    banti_draw_hiduke_sub(gfx_pp, (GAME*)play, &banti.day_anim, banti.displayed_time.day - 1, banti.next_time.day - 1,
                          1);
}

extern u8 clk_win_jikan0_TA_tex_txt[];
extern u8 clk_win_jikan1_TA_tex_txt[];
extern u8 clk_win_jikan2_TA_tex_txt[];
extern u8 clk_win_jikan3_TA_tex_txt[];
extern u8 clk_win_jikan4_TA_tex_txt[];
extern u8 clk_win_jikan5_TA_tex_txt[];
extern u8 clk_win_jikan6_TA_tex_txt[];
extern u8 clk_win_jikan7_TA_tex_txt[];
extern u8 clk_win_jikan8_TA_tex_txt[];
extern u8 clk_win_jikan9_TA_tex_txt[];
extern u8 clk_win_jikan_TA_tex_txt[];

static void banti_draw_jikan_sub(Gfx** gfx_pp, GAME* game, Banti_anim_c* anim, int idx0, int idx1, int hide_zero,
                                 int pos_idx) {
    static u8* jikan_tex_table[11] = { clk_win_jikan0_TA_tex_txt, clk_win_jikan1_TA_tex_txt, clk_win_jikan2_TA_tex_txt,
                                       clk_win_jikan3_TA_tex_txt, clk_win_jikan4_TA_tex_txt, clk_win_jikan5_TA_tex_txt,
                                       clk_win_jikan6_TA_tex_txt, clk_win_jikan7_TA_tex_txt, clk_win_jikan8_TA_tex_txt,
                                       clk_win_jikan9_TA_tex_txt, clk_win_jikan_TA_tex_txt };

    static xyz_t set_pos_table[4] = {
        { 60.0f, -91.0f, 0.0f }, { 76.0f, -91.0f, 0.0f }, { 98.0f, -91.0f, 0.0f }, { 113.0f, -91.0f, 0.0f }
    };

    GRAPH* g = game->graph;
    xyz_t* set_pos = &set_pos_table[pos_idx];
    Mtx* m;
    int joint;
    Banti_callback_c callback_data;

    Matrix_push();
    Matrix_translate(set_pos->x, set_pos->y, set_pos->z, MTX_MULT);

    if (hide_zero == TRUE) {
        if (idx0 == 0) {
            idx0 = 10;
        }

        if (idx1 == 0) {
            idx1 = 10;
        }
    }

    m = (Mtx*)GRAPH_ALLOC_TYPE(g, Mtx, anim->keyframe.skeleton->num_shown_joints);
    callback_data.mtx = m;

    if (m != NULL) {
        joint = 0;
        callback_data.gfx_pp = gfx_pp;
        callback_data.state = anim->state;

        gSPSegment(gfx_pp[0]++, G_MWO_SEGMENT_8, jikan_tex_table[idx0]);
        gSPSegment(gfx_pp[0]++, G_MWO_SEGMENT_9, jikan_tex_table[idx1]);

        cKF_Si3_draw_SV_R_child(game, &anim->keyframe, &joint, &banti_draw_before, NULL, &callback_data, NULL);
    }

    Matrix_pull();
}

static void banti_draw_jikan(Gfx** gfx_pp, GAME_PLAY* play, int alpha) {
    u8 a = alpha;
    int now_hour;
    int next_hour;

    gDPSetPrimColor(gfx_pp[0]++, 0, a, 255, 255, 255, a);
    gDPSetEnvColor(gfx_pp[0]++, 60, 25, 10, a);

    now_hour = banti.displayed_time.hour;
    if (now_hour == 0 || now_hour == 12) {
        now_hour = 12;
    } else if (now_hour > 11) {
        now_hour -= 12;
    }

    next_hour = banti.next_time.hour;
    if (next_hour == 0 || next_hour == 12) {
        next_hour = 12;
    } else if (next_hour > 11) {
        next_hour -= 12;
    }

    banti_draw_jikan_sub(gfx_pp, (GAME*)play, &banti.hour_upper_anim, now_hour / 10, next_hour / 10, TRUE, 0);
    banti_draw_jikan_sub(gfx_pp, (GAME*)play, &banti.hour_lower_anim, now_hour % 10, next_hour % 10, FALSE, 1);

    banti_draw_jikan_sub(gfx_pp, (GAME*)play, &banti.min_upper_anim, banti.displayed_time.min / 10,
                         banti.next_time.min / 10, FALSE, 2);
    banti_draw_jikan_sub(gfx_pp, (GAME*)play, &banti.min_lower_anim, banti.displayed_time.min % 10,
                         banti.next_time.min % 10, FALSE, 3);
}

extern u8 clk_win_pm_tex_rgb_ia8[];
extern u8 clk_win_am_tex_rgb_ia8[];

extern Gfx clk_win_mode[];

extern u8 clk_win_sun_tex_rgb_ia8[];
extern u8 clk_win_mon_tex_rgb_ia8[];
extern u8 clk_win_tue_tex_rgb_ia8[];
extern u8 clk_win_wed_tex_rgb_ia8[];
extern u8 clk_win_thu_tex_rgb_ia8[];
extern u8 clk_win_fri_tex_rgb_ia8[];
extern u8 clk_win_sat_tex_rgb_ia8[];

extern Gfx clk_win_youbiT_model[];
extern Gfx clk_win_maruT_model[];
extern Gfx clk_win_maru2T_model[];
extern Gfx clk_win_ampmT_model[];

extern void banti_draw(GAME_PLAY* play) {
    static u8* week_tex_table[lbRTC_WEEK] = { clk_win_sun_tex_rgb_ia8, clk_win_mon_tex_rgb_ia8, clk_win_tue_tex_rgb_ia8,
                                              clk_win_wed_tex_rgb_ia8, clk_win_thu_tex_rgb_ia8, clk_win_fri_tex_rgb_ia8,
                                              clk_win_sat_tex_rgb_ia8 };

    if (mFI_GET_TYPE(mFI_GetFieldId()) == mFI_FIELDTYPE_FG && mEv_CheckFirstIntro() != TRUE && banti.alpha > 0.01f) {
        GRAPH* g = play->game.graph;
        int poly_render = FALSE;
        Mtx* m = (Mtx*)GRAPH_ALLOC_TYPE(g, Mtx, 1);

        /* TODO: submenu mode enums */
        if (play->submenu.process_status != mSM_PROCESS_WAIT && play->submenu.mode > 2) {
            poly_render = TRUE;
        }

        OPEN_DISP(g);

        if (m != NULL) {
            mFont_CulcOrthoMatrix(m);

            if (poly_render == TRUE) {
                gSPMatrix(NOW_POLY_OPA_DISP++, m, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);
            } else {
                gSPMatrix(NOW_FONT_DISP++, m, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);
            }
        }

        Matrix_scale(16.0f, 16.0f, 16.0f, MTX_LOAD);

        if (banti.disp_left == TRUE) {
            Matrix_translate(-184.0f, 0.0f, 0.0f, MTX_MULT);
        }

        {
            Gfx** gfx_pp;
            u8* am_pm_tex = banti.displayed_time.hour < 12 ? clk_win_am_tex_rgb_ia8 : clk_win_pm_tex_rgb_ia8;
            int alpha;
            u8 a;
            Gfx* gfx;
            u32 rendermode0;
            u32 rendermode1;

            if (poly_render == TRUE) {
                gfx = NOW_POLY_OPA_DISP;
            } else {
                gfx = NOW_FONT_DISP;
            }

            gfx_pp = &gfx;

            gSPMatrix(gfx_pp[0]++, _Matrix_to_Mtx_new(g), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

            alpha = banti.alpha * 255.0f;
            if (banti.alpha >= 0.4f) {
                rendermode0 = G_RM_PASS;
                rendermode1 = G_RM_XLU_SURF2;
            } else {
                rendermode0 = G_RM_PASS;
                rendermode1 = G_RM_CLD_SURF2;
            }

            gSPDisplayList(gfx_pp[0]++, clk_win_mode);
            gDPSetRenderMode(gfx_pp[0]++, rendermode0, rendermode1);

            if (banti.displayed_time.weekday == lbRTC_SUNDAY) {
                u8 a = alpha;

                gDPSetPrimColor(gfx_pp[0]++, 0, a, 255, 255, 225, a);
                gDPSetEnvColor(gfx_pp[0]++, 165, 30, 30, a);
            } else {
                u8 a = alpha;

                gDPSetPrimColor(gfx_pp[0]++, 0, a, 255, 255, 225, a);
                gDPSetEnvColor(gfx_pp[0]++, 80, 40, 0, a);
            }

            gSPSegment(gfx_pp[0]++, G_MWO_SEGMENT_9, week_tex_table[banti.displayed_time.weekday]);
            gSPDisplayList(gfx_pp[0]++, clk_win_youbiT_model);

            if ((Common_Get(time.rtc_time).sec & 1) == 1) {
                u8 a = alpha;

                gDPSetPrimColor(gfx_pp[0]++, 0, a, 215, 120, 0, a);
                gDPSetEnvColor(gfx_pp[0]++, 70, 50, 50, a);
                gSPDisplayList(gfx_pp[0]++, clk_win_maru2T_model);
            }

            a = alpha;
            gDPSetPrimColor(gfx_pp[0]++, 0, a, 255, 255, 120, a);
            gDPSetEnvColor(gfx_pp[0]++, 70, 10, 10, a);
            gSPSegment(gfx_pp[0]++, G_MWO_SEGMENT_A, am_pm_tex);
            gSPDisplayList(gfx_pp[0]++, clk_win_ampmT_model);

            gDPSetPrimColor(gfx_pp[0]++, 0, a, 255, 255, 0, a);
            gDPSetEnvColor(gfx_pp[0]++, 70, 50, 50, a);
            gSPDisplayList(gfx_pp[0]++, clk_win_maruT_model);

            banti_draw_hiduke(&gfx, play, alpha);
            banti_draw_jikan(&gfx, play, alpha);

            if (poly_render == TRUE) {
                SET_POLY_OPA_DISP(*gfx_pp);
            } else {
                SET_FONT_DISP(*gfx_pp);
            }
        }

        CLOSE_DISP(g);
    }
}

#if VERSION >= VER_GAFU01_00
/* Deluxe: check if banti animation needs reset (84B)
 * Returns TRUE if the animation is currently playing and should
 * be reset (e.g., after time travel or reset stone interaction).
 * Resets animation to stopped state and returns the previous state. */
extern int banti_chk_reset_anime(Banti_anim_c* anim) {
    int was_playing = FALSE;

    if (anim->state == banti_ANIM_STATE_PLAYING) {
        was_playing = TRUE;
        anim->keyframe.frame_control.current_frame = 1.0f;
        anim->keyframe.frame_control.speed = 0.0f;
        anim->state = banti_ANIM_STATE_STOPPED;
    }

    return was_playing;
}
#endif

#if VERSION >= VER_DELUXE

/* Deluxe bulletin board display type */
static int banti_display_type;

/* Forward declarations for PP and N64 banti variants */
extern void banti_pp_ct(void);
extern void banti_pp_dt(void);
extern void banti_pp_move(GAME_PLAY* play);
extern void banti_pp_draw(GAME_PLAY* play);
extern void banti_64_ct(void);
extern void banti_64_dt(void);
extern void banti_64_move(GAME_PLAY* play);
extern void banti_64_draw(GAME_PLAY* play);

/**
 * @brief Update acre label vertex positions (60B).
 *
 * Adjusts vertex UV coordinates for the acre label display
 * on the bulletin board. Maps the digit index to texture
 * coordinates for the acre row/column indicator.
 *
 * @param vtx Vertex array to modify
 * @param u_offset Horizontal texture offset
 * @param v_offset Vertical texture offset
 */
extern void banti_update_acre_label_vertices(Vtx* vtx, int u_offset, int v_offset) {
    if (vtx != NULL) {
        vtx[0].v.tc[0] = (short)(u_offset * 16 * 32);
        vtx[0].v.tc[1] = (short)(v_offset * 16 * 32);
        vtx[1].v.tc[0] = (short)((u_offset + 1) * 16 * 32);
        vtx[1].v.tc[1] = (short)(v_offset * 16 * 32);
        vtx[2].v.tc[0] = (short)((u_offset + 1) * 16 * 32);
        vtx[2].v.tc[1] = (short)((v_offset + 1) * 16 * 32);
        vtx[3].v.tc[0] = (short)(u_offset * 16 * 32);
        vtx[3].v.tc[1] = (short)((v_offset + 1) * 16 * 32);
    }
}

/**
 * @brief Draw acre label on the bulletin board display (324B).
 *
 * Renders the current acre identifier (e.g., "A-3") on the
 * bulletin board HUD using the player's current field position.
 * Uses texture segments for row letter and column number.
 *
 * @param gfx_pp Display list pointer
 * @param play Game play state
 * @param alpha Display alpha value
 */
extern void banti_draw_acre_label(Gfx** gfx_pp, GAME_PLAY* play, int alpha) {
    GRAPH* g = play->game.graph;
    Vtx* vtx;
    Mtx* m;
    int acre_x, acre_z;
    int row, col;
    u8 a = (u8)alpha;

    if (mFI_GET_TYPE(mFI_GetFieldId()) != mFI_FIELDTYPE_FG) {
        return;
    }

    vtx = (Vtx*)GRAPH_ALLOC_TYPE(g, Vtx, 8);
    m = (Mtx*)GRAPH_ALLOC_TYPE(g, Mtx, 1);

    if (vtx == NULL || m == NULL) {
        return;
    }

    /* Get the player's acre position */
    {
        ACTOR* player = (ACTOR*)get_player_actor_withoutCheck(play);

        if (player == NULL) {
            return;
        }

        acre_x = (int)(player->world.position.x / mFI_BK_WORLDSIZE_X_F);
        acre_z = (int)(player->world.position.z / mFI_BK_WORLDSIZE_Z_F);
    }

    /* Clamp to valid range */
    if (acre_x < 0) acre_x = 0;
    if (acre_x > 6) acre_x = 6;
    if (acre_z < 0) acre_z = 0;
    if (acre_z > 6) acre_z = 6;

    row = acre_z;  /* A-G rows */
    col = acre_x;  /* 1-7 columns */

    /* Set up row letter vertices */
    {
        int i;
        short base_x = 120;
        short base_y = -55;

        for (i = 0; i < 4; i++) {
            vtx[i].v.ob[0] = base_x + ((i & 1) ? 16 : 0);
            vtx[i].v.ob[1] = base_y + ((i >= 2) ? -16 : 0);
            vtx[i].v.ob[2] = 0;
            vtx[i].v.flag = 0;
            vtx[i].v.cn[0] = 255;
            vtx[i].v.cn[1] = 255;
            vtx[i].v.cn[2] = 255;
            vtx[i].v.cn[3] = a;
        }
    }

    /* Set up column number vertices */
    {
        int i;
        short base_x = 140;
        short base_y = -55;

        for (i = 0; i < 4; i++) {
            vtx[4 + i].v.ob[0] = base_x + ((i & 1) ? 16 : 0);
            vtx[4 + i].v.ob[1] = base_y + ((i >= 2) ? -16 : 0);
            vtx[4 + i].v.ob[2] = 0;
            vtx[4 + i].v.flag = 0;
            vtx[4 + i].v.cn[0] = 255;
            vtx[4 + i].v.cn[1] = 255;
            vtx[4 + i].v.cn[2] = 255;
            vtx[4 + i].v.cn[3] = a;
        }
    }

    /* Set texture coordinates for row and column */
    banti_update_acre_label_vertices(&vtx[0], row, 0);
    banti_update_acre_label_vertices(&vtx[4], col, 1);

    _Matrix_to_Mtx(m);

    gSPMatrix(gfx_pp[0]++, m, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gDPSetPrimColor(gfx_pp[0]++, 0, a, 255, 255, 200, a);
    gDPSetEnvColor(gfx_pp[0]++, 60, 30, 10, a);
}

/**
 * @brief Format hour for display (56B).
 *
 * Converts a 24-hour value to 12-hour format for the
 * bulletin board clock display.
 *
 * @param hour 24-hour format hour (0-23)
 * @return 12-hour format hour (1-12)
 */
extern int banti_fmt_hr(int hour) {
    if (hour == 0 || hour == 12) {
        return 12;
    } else if (hour > 12) {
        return hour - 12;
    }

    return hour;
}

/**
 * @brief Check and initialize animation for digit change (72B).
 *
 * Compares current and target values. If they differ and the
 * animation is currently stopped, starts the transition animation.
 *
 * @param anim Banti animation state
 * @param current Current displayed value
 * @param target Target value to transition to
 */
extern void banti_chk_init_anim(Banti_anim_c* anim, int current, int target) {
    if (anim->state == banti_ANIM_STATE_STOPPED && current != target) {
        banti_animation_init_hiduke(anim);
        anim->state = banti_ANIM_STATE_PLAYING;
    }
}

/**
 * @brief Check and init tens digit animation (44B).
 *
 * Compares the tens digits of two values and starts animation
 * if they differ.
 *
 * @param anim Banti animation state
 * @param current Current value
 * @param target Target value
 */
extern void banti_chk_init_tens_anim(Banti_anim_c* anim, int current, int target) {
    banti_chk_init_anim(anim, current / 10, target / 10);
}

/**
 * @brief Check and init ones digit animation (60B).
 *
 * Compares the ones digits of two values and starts animation
 * if they differ.
 *
 * @param anim Banti animation state
 * @param current Current value
 * @param target Target value
 */
extern void banti_chk_init_ones_anim(Banti_anim_c* anim, int current, int target) {
    int cur_ones = current - (current / 10) * 10;
    int tgt_ones = target - (target / 10) * 10;

    banti_chk_init_anim(anim, cur_ones, tgt_ones);
}

/**
 * @brief Bulletin board constructor (84B).
 *
 * Wrapper constructor that dispatches to the appropriate banti
 * variant (GC, PP, or N64) based on the current display type.
 * Initializes the display type and calls the variant constructor.
 */
extern void mBanti_ct(void) {
    banti_display_type = 0;

    switch (banti_display_type) {
        case 0:
            banti_ct();
            break;
        case 1:
            banti_pp_ct();
            break;
        case 2:
            banti_64_ct();
            break;
    }
}

/**
 * @brief Bulletin board destructor (72B).
 *
 * Wrapper destructor that dispatches to the appropriate banti
 * variant based on the current display type.
 */
extern void mBanti_dt(void) {
    switch (banti_display_type) {
        case 0:
            banti_dt();
            break;
        case 1:
            banti_pp_dt();
            break;
        case 2:
            banti_64_dt();
            break;
    }
}

/**
 * @brief Bulletin board move/update (60B).
 *
 * Wrapper move function that dispatches to the appropriate banti
 * variant based on the current display type.
 */
extern void mBanti_mv(GAME_PLAY* play) {
    switch (banti_display_type) {
        case 0:
            banti_move(play);
            break;
        case 1:
            banti_pp_move(play);
            break;
        case 2:
            banti_64_move(play);
            break;
    }
}

/**
 * @brief Bulletin board draw (60B).
 *
 * Wrapper draw function that dispatches to the appropriate banti
 * variant based on the current display type.
 */
extern void mBanti_dw(GAME_PLAY* play) {
    switch (banti_display_type) {
        case 0:
            banti_draw(play);
            break;
        case 1:
            banti_pp_draw(play);
            break;
        case 2:
            banti_64_draw(play);
            break;
    }
}

/**
 * @brief Set bulletin board display type (68B).
 *
 * Reads the current game configuration and selects the appropriate
 * banti display variant. If the type changes, destroys the current
 * variant and constructs the new one. Types:
 *   0 = GC-native (skeleton keyframe animation)
 *   1 = PP (piracy protection banner style)
 *   2 = N64 (N64-era GBI display list style)
 */
extern void banti_set_type(void) {
    int new_type = 0; /* Default: GC-native */
    GAME_PLAY* play = (GAME_PLAY*)gamePT;

    if (play != NULL) {
        /* Select type based on game state */
        if (Common_Get(time.rtc_time).year >= 2004) {
            new_type = 1; /* PP style for Deluxe year range */
        }
    }

    if (new_type != banti_display_type) {
        /* Destroy current */
        switch (banti_display_type) {
            case 0:
                banti_dt();
                break;
            case 1:
                banti_pp_dt();
                break;
            case 2:
                banti_64_dt();
                break;
        }

        banti_display_type = new_type;

        /* Construct new */
        switch (banti_display_type) {
            case 0:
                banti_ct();
                break;
            case 1:
                banti_pp_ct();
                break;
            case 2:
                banti_64_ct();
                break;
        }
    }
}

#endif /* VERSION >= VER_DELUXE */
