#include "m_submenu.h"
#include "m_submenu_ovl.h"

#include "m_common_data.h"
#include "m_name_table.h"
#include "m_player_lib.h"
#include "m_msg.h"
#include "m_quest.h"
#include "m_card.h"
#include "m_event.h"
#include "m_demo.h"
#include "m_museum_display.h"
#include "libultra/libultra.h"

static mSM_dlftbl_c SubmenuArea_dlftbl[mSM_DLF_NUM] = { { NULL, 0, 0, 0, 0, 0, "submenu_ovl" },
                                                        { NULL, 0, 0, 0, 0, 0, "player_actor" } };

static mSM_dlftbl_c* SubmenuArea_visit = NULL;
static void* SubmenuArea_allocp = NULL;

extern int mSM_COLLECT_INSECT_GET(int idx) {
    u32 ftr_idx = (0x2F2 << 2) + (idx << 2);
    return (Common_Get(now_private)->furniture_collected_bitfield[ftr_idx >> 2 >> 5] &
            (1 << ((ftr_idx >> 2) & 0x1F))) != 0;
}

extern void mSM_COLLECT_INSECT_SET(int idx) {
    u32 ftr_idx = (0x2F2 << 2) + (idx << 2);
    Common_Get(now_private)->furniture_collected_bitfield[ftr_idx >> 2 >> 5] |= (1 << ((ftr_idx >> 2) & 0x1F));
}

static int mSM_CHECK_ALL_INSECT_GET_SUB() {
    int i;
    int n = 0;

    for (i = 0; i < INSECT_ONLY_NUM; i++) {
        if (mSM_COLLECT_INSECT_GET(i) != FALSE) {
            n++;
        }
    }

    return n;
}

extern int mSM_CHECK_ALL_INSECT_GET() {
    if (mSM_CHECK_ALL_INSECT_GET_SUB() == INSECT_ONLY_NUM) {
        return TRUE;
    }

    return FALSE;
}

extern int mSM_CHECK_LAST_INSECT_GET(int idx) {
    int res = FALSE;

    if (mSM_CHECK_ALL_INSECT_GET_SUB() == (INSECT_ONLY_NUM - 1) && mSM_COLLECT_INSECT_GET(idx) == FALSE) {
        res = TRUE;
    }

    return res;
}

extern int mSM_COLLECT_FISH_GET(int idx) {
    u32 ftr_idx = (0x31A << 2) + (idx << 2);
    return (Common_Get(now_private)->furniture_collected_bitfield[ftr_idx >> 2 >> 5] &
            (1 << ((ftr_idx >> 2) & 0x1F))) != 0;
}

extern void mSM_COLLECT_FISH_SET(int idx) {
    u32 ftr_idx = (0x31A << 2) + (idx << 2);
    Common_Get(now_private)->furniture_collected_bitfield[ftr_idx >> 2 >> 5] |= (1 << ((ftr_idx >> 2) & 0x1F));
}

static int mSM_CHECK_ALL_FISH_GET_SUB() {
    int i;
    int n = 0;

    for (i = 0; i < FISH_NUM; i++) {
        if (mSM_COLLECT_FISH_GET(i) != FALSE) {
            n++;
        }
    }

    return n;
}

extern int mSM_CHECK_ALL_FISH_GET() {
    if (mSM_CHECK_ALL_FISH_GET_SUB() == FISH_NUM) {
        return TRUE;
    }

    return FALSE;
}

extern int mSM_CHECK_LAST_FISH_GET(int idx) {
    int res = FALSE;

    if (mSM_CHECK_ALL_FISH_GET_SUB() == (FISH_NUM - 1) && mSM_COLLECT_FISH_GET(idx) == FALSE) {
        res = TRUE;
    }

    return res;
}

extern int SubmenuArea_IsPlayer() {
    return SubmenuArea_visit == &SubmenuArea_dlftbl[mSM_DLF_PLAYER_ACTOR];
}

static void mSM_load_player_anime(GAME_PLAY* play) {
    // stubbed
}

static void SubmenuArea_DoLink(mSM_dlftbl_c* dlftbl, Submenu* submenu, int dlf_idx) {
    dlftbl->_00 = SubmenuArea_allocp;
    dlftbl->_14 = 0;
    SubmenuArea_visit = dlftbl;
    submenu->overlay_address = dlftbl->_00;
    submenu->next_overlay_address = dlftbl->_00;
}

static void SubmenuArea_DoUnlink(mSM_dlftbl_c* dlftbl, Submenu* submenu) {
    if (dlftbl->_00 != NULL) {
        submenu->overlay_address = dlftbl->_00;
        submenu->next_overlay_address = dlftbl->_00;
        dlftbl->_14 = 0;
        dlftbl->_00 = NULL;
        SubmenuArea_visit = NULL;
    }
}

static int mSM_ovlptr_dllcnv_sub(void* proc, mSM_dlftbl_c* dlftbl, Submenu* submenu, int dlf_idx) {
    if (dlf_idx == mSM_DLF_SUBMENU_OVL || dlf_idx == mSM_DLF_PLAYER_ACTOR) {
        SubmenuArea_DoLink(dlftbl, submenu, dlf_idx);
        return TRUE;
    }

    return FALSE;
}

extern void* mSM_ovlptr_dllcnv(void* proc, Submenu* submenu, int dlf_idx) {
#ifdef TARGET_PC
    /* On PC all code is statically linked — no overlay resolution needed */
    if (SubmenuArea_visit == NULL) {
        mSM_ovlptr_dllcnv_sub(proc, SubmenuArea_dlftbl, submenu, dlf_idx);
    }
    return proc;
#else
    if (SubmenuArea_visit != NULL) {
        return; // ??
    } else if (mSM_ovlptr_dllcnv_sub(proc, SubmenuArea_dlftbl, submenu, dlf_idx) == FALSE) {
        return NULL;
    }

    return proc;
#endif
}

extern void mSM_submenu_ovlptr_init(GAME_PLAY* play) {
    SubmenuArea_allocp = (void*)1;
    SubmenuArea_visit = NULL;
}

extern void mSM_submenu_ovlptr_cleanup(Submenu* submenu) {
    if (SubmenuArea_visit != NULL) {
        SubmenuArea_DoUnlink(SubmenuArea_visit, submenu);
        SubmenuArea_visit = NULL;
    }

    SubmenuArea_allocp = NULL;
}

extern void load_player(Submenu* submenu) {
    mSM_dlftbl_c* dlftbl = &SubmenuArea_dlftbl[mSM_DLF_PLAYER_ACTOR];

    if (SubmenuArea_visit != dlftbl) {
        if (SubmenuArea_visit != NULL) {
            SubmenuArea_DoUnlink(SubmenuArea_visit, submenu);
        }

        SubmenuArea_DoLink(dlftbl, submenu, mSM_DLF_PLAYER_ACTOR);
    }
}

extern void mSM_submenu_ct(Submenu* submenu) {
    bzero(submenu, sizeof(Submenu));
    submenu->process_status = mSM_PROCESS_WAIT;
    submenu->wait_timer = 0;

    if (Common_Get(submenu_disabled) == TRUE) {
        submenu->start_refuse = TRUE;
        Common_Set(submenu_disabled, FALSE);
    }

    submenu->move_proc = (SUBMENU_PROC)&none_proc1;
    submenu->draw_proc = (SUBMENU_GAME_PROC)&none_proc1;
}

extern void mSM_submenu_dt(Submenu* submenu) {
    // stubbed
}

extern void mSM_open_submenu(Submenu* submenu, int type, int arg0, int arg1) {
    mSM_open_submenu_new2(submenu, type, arg0, arg1, NULL, 0);
}

extern void mSM_open_submenu_new(Submenu* submenu, int type, int arg0, int arg1, void* arg2) {
    mSM_open_submenu_new2(submenu, type, arg0, arg1, arg2, 0);
}

extern void mSM_open_submenu_new2(Submenu* submenu, int type, int arg0, int arg1, void* arg2, int arg3) {
    submenu->menu_type = type;
    submenu->param0 = arg0;
    submenu->param1 = arg1;
    submenu->param2 = arg2;
    submenu->param3 = arg3;
}

static void mSM_Reset_player_btn_type1(GAME_PLAY* play) {
    PLAYER_ACTOR* player = GET_PLAYER_ACTOR(play);

    if (player != NULL) {
        player->a_btn_pressed = FALSE;
        player->a_btn_triggers_submenu = TRUE;
    }
}

static void mSM_Reset_player_btn_type2(GAME_PLAY* play) {
    PLAYER_ACTOR* player = GET_PLAYER_ACTOR(play);

    if (player != NULL) {
        player->a_btn_triggers_submenu = TRUE;
    }
}

static int mSM_check_open_map_new(GAME_PLAY* play) {
    PLAYER_ACTOR* player = GET_PLAYER_ACTOR(play);
    int open_map = FALSE;
    int bx;
    int bz;

    if (mSM_CHECK_OPEN_MAP()) {
        open_map = TRUE;
    }

    if (open_map == FALSE) {
        return FALSE;
    }

    if (player == NULL) {
        return FALSE;
    }

    if (Save_Get(scene_no) == SCENE_COTTAGE_MY || Save_Get(scene_no) == SCENE_COTTAGE_NPC) {
        return FALSE;
    }

    mFI_Wpos2BlockNum(&bx, &bz, player->actor_class.world.position);
    return mFI_CheckBlockKind_OR(bx, bz, mRF_BLOCKKIND_OCEAN) == FALSE;
}

extern void mSM_submenu_ctrl(GAME_PLAY* play) {
    Submenu* submenu = &play->submenu;
    int open_inventory;

    if (submenu->process_status != mSM_PROCESS_WAIT) {
        return;
    }

    if (play->fb_fade_type != FADE_TYPE_NONE) {
        return;
    }

    if (play->fb_wipe_mode != WIPE_MODE_NONE) {
        return;
    }

    open_inventory = FALSE;
    if (chkTrigger(mSM_INV_BUTTON_0) || chkTrigger(mSM_INV_BUTTON_1)) {
        open_inventory = TRUE;
    }

    if (((open_inventory && Common_Get(reset_flag) == FALSE) ||
         (mSM_check_open_map_new(play) == TRUE && Common_Get(reset_flag) == FALSE)) &&
        submenu->start_refuse == FALSE && submenu->start_refuse_timer == 0 && mPlib_able_submenu_type1((GAME*)play) &&
        mEv_CheckFirstIntro() == FALSE) {
        if (open_inventory) {
            mSM_open_submenu(submenu, mSM_OVL_INVENTORY, 0, 0);
        } else {
            mSM_open_submenu(submenu, mSM_OVL_MAP, 1, 0);
        }

        mSM_Reset_player_btn_type2(play);
    } else {
        PLAYER_ACTOR* player = GET_PLAYER_ACTOR(play);

        if (player != NULL && player->a_btn_pressed == TRUE && Common_Get(reset_flag) == FALSE &&
            submenu->start_refuse == FALSE && submenu->start_refuse_timer == 0 &&
            mPlib_able_submenu_type1((GAME*)play)) {
            s16 y_dir = player->actor_class.shape_info.rotation.y;
            int dir;

            y_dir -= -0x8000;
            dir = ABS(y_dir);

            if (dir < DEG2SHORT_ANGLE(45.0f)) {
                switch (player->item_in_front) {
                    case MESSAGE_BOARD1:
                    case MESSAGE_BOARD0:
                        mSM_open_submenu(submenu, mSM_OVL_NOTICE, 0, 0);
                        mSM_Reset_player_btn_type1(play);
                        break;
                    case MAP_BOARD1:
                    case MAP_BOARD0:
                        mSM_open_submenu(submenu, mSM_OVL_MAP, 0, 0);
                        mSM_Reset_player_btn_type1(play);
                        break;
                }
            }
        }
    }

    if (submenu->menu_type != mSM_OVL_NONE) {
        submenu->process_status = mSM_PROCESS_PREWAIT;
        submenu->mode = mSM_MODE_PRERENDER_INIT;
        SetGameFrame(1);
    }
}

static void mSM_move_Wait(Submenu* submenu) {
    if (submenu->wait_timer != 0) {
        submenu->wait_timer--;
    }

    if (submenu->start_refuse_timer != 0) {
        submenu->start_refuse_timer--;
    }
}

static void mSM_move_PREWait(Submenu* submenu) {
    if (submenu->mode >= mSM_MODE_PRERENDER_DONE) {
        submenu->process_status = mSM_PROCESS_LINKWAIT;
    }
}

static void mSM_move_LINKWait(Submenu* submenu) {
    Submenu_Item_c* item;
    mSM_dlftbl_c* dlftbl = &SubmenuArea_dlftbl[mSM_DLF_SUBMENU_OVL];
    int i;

    if (SubmenuArea_visit != dlftbl) {
        if (SubmenuArea_visit != NULL) {
            SubmenuArea_DoUnlink(SubmenuArea_visit, submenu);
        }

        SubmenuArea_DoLink(dlftbl, submenu, mSM_DLF_SUBMENU_OVL);
        submenu->move_proc = (SUBMENU_PROC)mSM_ovlptr_dllcnv(&mSM_menu_ovl_init, submenu, mSM_DLF_SUBMENU_OVL);
        submenu->draw_proc = (SUBMENU_GAME_PROC)&none_proc1;
        submenu->process_status = mSM_PROCESS_PLAY;
        submenu->open_flag = TRUE;
        submenu->after_mode = 7;
        submenu->unk_164 = 0;
        mMl_clear_mail(&submenu->mail);
        submenu->item_p = &submenu->items[0];
        submenu->item_num = mPr_POCKETS_SLOT_COUNT;

        item = submenu->item_p;
        for (i = 0; i < submenu->item_num; i++) {
            item->item = EMPTY_NO;
            item->slot_no = mPr_POCKETS_SLOT_COUNT;
            item++;
        }

        if (submenu->mode != mSM_MODE_OTHER) {
            if ((submenu->menu_type == mSM_OVL_LEDIT && submenu->param0 == 0) ||
                Common_Get(now_private)->gender == mPr_SEX_MALE) {
                sAdo_SpecChange(5);
            } else {
                sAdo_SpecChange(6);
            }

            sAdo_SetVoiceMode(0);
        }
    }
}

static void mSM_move_Play(Submenu* submenu) {
    (*submenu->move_proc)(submenu);
}

static void mSM_move_End(Submenu* submenu) {
    mSM_dlftbl_c* dlftbl = &SubmenuArea_dlftbl[mSM_DLF_SUBMENU_OVL];
    GAME_PLAY* play = (GAME_PLAY*)gamePT;

    (*submenu->move_proc)(submenu);
    submenu->process_status = mSM_PROCESS_WAIT;
    submenu->menu_type = mSM_OVL_NONE;
    submenu->wait_timer = 2;
    submenu->open_flag = FALSE;
    SetGameFrame(1);

    if (submenu->mode != mSM_MODE_OTHER) {
        mMsg_Window_c* msg_win = mMsg_Get_base_window_p();
        submenu->mode = mSM_MODE_IDLE;
        mSc_dmacopy_all_exchange_bank(&play->object_exchange);
        SubmenuArea_DoUnlink(dlftbl, submenu);
        load_player(submenu);
        mSM_load_player_anime(play);
        submenu->start_refuse_timer = 1;

        if (submenu->after_mode == 12) {
            mPlib_request_main_demo_wait_from_submenu((ACTOR*)submenu->overlay->menu_info[mSM_OVL_MSCORE].data2);
        }

        if (mMsg_Check_main_hide(msg_win) == FALSE && mMsg_Check_not_series_main_wait(msg_win)) {
            mMsg_sound_spec_change_voice(msg_win);
        }
    }
}

extern void mSM_submenu_move(Submenu* submenu) {
    static SUBMENU_PROC move_proc[mSM_PROCESS_NUM] = { &mSM_move_Wait, &mSM_move_PREWait, &mSM_move_LINKWait,
                                                       &mSM_move_Play, &mSM_move_End };

    (*move_proc[submenu->process_status])(submenu);
}

extern void mSM_submenu_draw(Submenu* submenu, GAME* game) {
    if (submenu->mode >= mSM_MODE_PRERENDER_DONE && submenu->process_status == mSM_PROCESS_PLAY &&
        SubmenuArea_visit == &SubmenuArea_dlftbl[mSM_DLF_SUBMENU_OVL]) {
        (*submenu->draw_proc)(submenu, game);
    }
}

static int mSM_check_item_for_furniture(int slot_no, int param_2) {
    Private_c* priv = Common_Get(now_private);
    mActor_name_t item = priv->inventory.pockets[slot_no];
    int cat = ITEM_NAME_GET_CAT(item);
    int res = FALSE;

    if (item != EMPTY_NO && mPr_GET_ITEM_COND(priv->inventory.item_conditions, slot_no) == mPr_ITEM_COND_NORMAL &&
        ITEM_NAME_GET_TYPE(item) == NAME_TYPE_ITEM1 && cat != ITEM1_CAT_FISH && cat != ITEM1_CAT_KABU &&
        cat != ITEM1_CAT_INSECT && item != ITM_KNIFE_AND_FORK &&
        !(item >= ITM_EXCERCISE_CARD00 && item <= ITM_EXCERCISE_CARD12)) {
        res = TRUE;
    }

    return res;
}

static int mSM_check_item_for_quest(int slot_no, int param_2) {
    Private_c* priv = Common_Get(now_private);
    mActor_name_t item = priv->inventory.pockets[slot_no];
    int res = FALSE;

    if (item != EMPTY_NO && !(item >= ITM_EXCERCISE_CARD00 && item <= ITM_EXCERCISE_CARD12) &&
        item != ITM_KNIFE_AND_FORK && !(item >= ITM_SPIRIT0 && item <= ITM_SPIRIT4)) {
        res = TRUE;
    }

    return res;
}

static int mSM_check_item_for_sell(int slot_no, int param_2) {
    Private_c* priv = Common_Get(now_private);
    mActor_name_t item = priv->inventory.pockets[slot_no];
    int res = FALSE;

    if (item != EMPTY_NO && mPr_GET_ITEM_COND(priv->inventory.item_conditions, slot_no) == mPr_ITEM_COND_NORMAL &&
        (ITEM_NAME_GET_TYPE(item) != NAME_TYPE_ITEM1 || ITEM_NAME_GET_CAT(item) != ITEM1_CAT_MONEY) &&
        !(item >= ITM_EXCERCISE_CARD00 && item <= ITM_EXCERCISE_CARD12) && item != ITM_KNIFE_AND_FORK &&
        !(item >= ITM_SPIRIT0 && item <= ITM_SPIRIT4)) {
        res = TRUE;
    }

    return res;
}

static int mSM_check_item_for_give(int slot_no, int param_2) {
    Private_c* priv = Common_Get(now_private);
    mActor_name_t item = priv->inventory.pockets[slot_no];
    int res = FALSE;

    if (item != EMPTY_NO && mPr_GET_ITEM_COND(priv->inventory.item_conditions, slot_no) == mPr_ITEM_COND_NORMAL &&
        !(item >= ITM_EXCERCISE_CARD00 && item <= ITM_EXCERCISE_CARD12) && item != ITM_KNIFE_AND_FORK &&
        !(item >= ITM_SPIRIT0 && item <= ITM_SPIRIT4)) {
        res = TRUE;
    }

    return res;
}

static int mSM_check_item_for_take(int slot_no, int param_2) {
    Private_c* priv = Common_Get(now_private);
    mActor_name_t item = priv->inventory.pockets[slot_no];
    int cat = ITEM_NAME_GET_CAT(item);
    int res = FALSE;

    if (item != EMPTY_NO && mPr_GET_ITEM_COND(priv->inventory.item_conditions, slot_no) == mPr_ITEM_COND_NORMAL &&
        !(item >= ITM_EXCERCISE_CARD00 && item <= ITM_EXCERCISE_CARD12) && item != ITM_KNIFE_AND_FORK &&
        !(item >= ITM_SPIRIT0 && item <= ITM_SPIRIT4) &&
        (param_2 == 0 || (ITEM_NAME_GET_TYPE(item) == NAME_TYPE_ITEM1 &&
                          ((cat == ITEM1_CAT_FISH && param_2 == 1) || (cat == ITEM1_CAT_INSECT && param_2 == 2))))) {
        res = TRUE;
    }

    return res;
}

static int mSM_check_item_for_minidisk(int slot_no, int param_2) {
    Private_c* priv = Common_Get(now_private);
    mActor_name_t item = priv->inventory.pockets[slot_no];

    if (mPr_GET_ITEM_COND(priv->inventory.item_conditions, slot_no) == mPr_ITEM_COND_NORMAL &&
        ITEM_NAME_GET_TYPE(item) == NAME_TYPE_ITEM1 && ITEM_NAME_GET_CAT(item) == ITEM1_CAT_MINIDISK) {
        return TRUE;
    }

    return FALSE;
}

static int mSM_check_item_for_shrine(int slot_no, int param_2) {
    Private_c* priv = Common_Get(now_private);
    if (mPr_GET_ITEM_COND(priv->inventory.item_conditions, slot_no) == mPr_ITEM_COND_QUEST &&
        mQst_CheckLimitbyPossessionIdx(slot_no)) {
        return TRUE;
    }

    return FALSE;
}

static int mSM_check_item_for_entrust(int slot_no, int param_2) {
    Private_c* priv = Common_Get(now_private);
    mActor_name_t item = priv->inventory.pockets[slot_no];
    int res = FALSE;

    if (item == EMPTY_NO ||
        mPr_GET_ITEM_COND(priv->inventory.item_conditions, slot_no) == mPr_ITEM_COND_NORMAL &&
            (ITEM_NAME_GET_TYPE(item) != NAME_TYPE_ITEM1 || ITEM_NAME_GET_CAT(item) != ITEM1_CAT_MONEY) &&
            !(item >= ITM_EXCERCISE_CARD00 && item <= ITM_EXCERCISE_CARD12) && item != ITM_KNIFE_AND_FORK &&
            !(item >= ITM_SPIRIT0 && item <= ITM_SPIRIT4)) {
        res = TRUE;
    }

    return res;
}

static int mSM_check_item_for_exchange(int slot_no, int exchange_id) {
    int res = FALSE;
    Private_c* priv = Common_Get(now_private);
    mActor_name_t item = priv->inventory.pockets[slot_no];

    if (item != EMPTY_NO && mPr_GET_ITEM_COND(priv->inventory.item_conditions, slot_no) == mPr_ITEM_COND_NORMAL &&
        item != ITM_SIGNBOARD && item != ITM_KNIFE_AND_FORK &&
        !(item >= ITM_EXCERCISE_CARD00 && item <= ITM_EXCERCISE_CARD12)) {
        if ((ITEM_NAME_GET_TYPE(item) == NAME_TYPE_ITEM1 && ITEM_NAME_GET_CAT(item) == ITEM1_CAT_FISH) &&
            (ITEM_NAME_GET_TYPE(exchange_id) != NAME_TYPE_ITEM1 || ITEM_NAME_GET_CAT(exchange_id) != ITEM1_CAT_FISH)) {
            PLAYER_ACTOR* player = GET_PLAYER_ACTOR_NOW();
            xyz_t water_pos;

            if (mCoBG_SearchWaterLimitDistN(&water_pos, player->actor_class.world.position,
                                            player->actor_class.shape_info.rotation.y, 120.0f, 12) == FALSE) {
                return FALSE;
            }

            xyz_t_move(&((GAME_PLAY*)gamePT)->submenu.water_pos, &water_pos);
        }

        res = TRUE;
    }

    return res;
}

static int mSM_check_item_for_curator(int slot_no, int param_2) {
    Private_c* priv = Common_Get(now_private);
    mActor_name_t item = priv->inventory.pockets[slot_no];
    int res = FALSE;

    if (item != EMPTY_NO && mPr_GET_ITEM_COND(priv->inventory.item_conditions, slot_no) == mPr_ITEM_COND_NORMAL &&
        item != ITM_KNIFE_AND_FORK && !(item >= ITM_EXCERCISE_CARD00 && item <= ITM_EXCERCISE_CARD12)) {
#if VERSION >= VER_DELUXE
        /* Deluxe: only show items the curator can actually accept.
         * Filter out already-donated items and non-donatable items. */
        if (item == ITM_FOSSIL) {
            /* Unidentified fossils are always selectable for the curator to identify */
            res = TRUE;
        } else {
            int display_info = mMmd_GetDisplayInfo(item);

            if (display_info == mMmd_DISPLAY_CAN_DONATE) {
                /* Item is a valid donation type and hasn't been donated yet */
                res = TRUE;
            }
            /* mMmd_DISPLAY_ALREADY_DONATED: greyed out (already in museum) */
            /* mMmd_DISPLAY_CANNOT_DONATE: greyed out (not a museum item) */
        }
#else
        res = TRUE;
#endif
    }

    return res;
}

typedef int (*mSM_INVENTORY_CHECK_PROC)(int, int);

extern int mSM_check_open_inventory_itemlist(int type, int param_2) {
    static mSM_INVENTORY_CHECK_PROC check_process[mSM_IV_OPEN_NUM] = { NULL,
                                                                       NULL,
                                                                       &mSM_check_item_for_entrust,
                                                                       NULL,
                                                                       &mSM_check_item_for_quest,
                                                                       &mSM_check_item_for_sell,
                                                                       &mSM_check_item_for_give,
                                                                       NULL,
                                                                       &mSM_check_item_for_take,
                                                                       &mSM_check_item_for_furniture,
                                                                       &mSM_check_item_for_minidisk,
                                                                       &mSM_check_item_for_shrine,
                                                                       NULL,
                                                                       &mSM_check_item_for_exchange,
                                                                       NULL,
                                                                       &mSM_check_item_for_curator,
                                                                       NULL };

    mSM_INVENTORY_CHECK_PROC check_proc = check_process[type];
    int i;
    int res = 0;

    if (check_proc == NULL) {
        return 0xFFFF; /* all slots are valid */
    }

    for (i = 0; i < mPr_POCKETS_SLOT_COUNT; i++) {
        if ((*check_proc)(i, param_2) != FALSE) {
            res |= (1 << i); // slot is valid
        }
    }

    return res;
}

static char* mSM_Object_Exchange_keep_new(GAME_PLAY* play, s16 bank_id, size_t size) {
    Object_Exchange_c* exchange = &play->object_exchange;
    Object_Bank_c* bank = &exchange->banks[exchange->bank_idx];

    bank->bank_id = bank_id;
    bank->dma_start = exchange->next_bank_ram_address;
    bank->ram_start = exchange->next_bank_ram_address;
    bank->size = size;

    if (exchange->bank_idx < mSc_OBJECT_BANK_NUM - 1) {
        exchange->next_bank_ram_address = (char*)ALIGN_NEXT((u32)(exchange->next_bank_ram_address + size), 16);
        exchange->bank_idx++;
    }

    return bank->dma_start;
}

static void mSM_Object_Exchange_keep_new_MenuTexAndPallet(GAME_PLAY* play, int idx) {
    char* tex_p = mSM_Object_Exchange_keep_new(play, ACTOR_OBJ_BANK_14, (32 * 32) / 2);
    char* pal_p = mSM_Object_Exchange_keep_new(play, ACTOR_OBJ_BANK_15, 16 * sizeof(u16));

    play->submenu_ground_tex[idx] = tex_p;
    play->submenu_ground_pallet[idx] = pal_p;
    mPlib_Load_PlayerTexAndPallet(tex_p, pal_p, Common_Get(now_private)->backgound_texture - ITM_CLOTH_START);
}

extern void mSM_Object_Exchange_keep_new_Menu(GAME_PLAY* play) {
    play->submenu_ground_idx = 0;
    mSM_Object_Exchange_keep_new_MenuTexAndPallet(play, 0);
    mSM_Object_Exchange_keep_new_MenuTexAndPallet(play, 1);
}

extern u8* mSM_Get_ground_tex_p(GAME_PLAY* play) {
    int idx = play->submenu_ground_idx;

    if (idx < 0) {
        return NULL;
    }

    return (u8*)play->submenu_ground_tex[idx];
}

extern u16* mSM_Get_ground_pallet_p(GAME_PLAY* play) {
    int idx = play->submenu_ground_idx;

    if (idx < 0) {
        return NULL;
    }

    return (u16*)play->submenu_ground_pallet[idx];
}

#if VERSION >= VER_DELUXE
/* Deluxe: check if player can interact with foreground items (188B).
 * Validates player state, event status, screen transitions, and submenu availability. */
extern int mSM_check_open_FG(GAME_PLAY* play) {
    PLAYER_ACTOR* player = GET_PLAYER_ACTOR(play);

    if (player == NULL) {
        return FALSE;
    }

    if (Common_Get(reset_flag) != FALSE) {
        return FALSE;
    }

    if (mEv_CheckFirstIntro() != FALSE) {
        return FALSE;
    }

    if (mEv_CheckFirstJob() != FALSE) {
        return FALSE;
    }

    if (mDemo_CheckDemo() != FALSE) {
        return FALSE;
    }

    if (play->fb_fade_type != FADE_TYPE_NONE) {
        return FALSE;
    }

    if (play->fb_wipe_mode != WIPE_MODE_NONE) {
        return FALSE;
    }

    if (play->submenu.process_status != mSM_PROCESS_WAIT) {
        return FALSE;
    }

    if (play->submenu.start_refuse != FALSE) {
        return FALSE;
    }

    if (mPlib_able_submenu_type1((GAME*)play) == FALSE) {
        return FALSE;
    }

    return TRUE;
}
#endif

/* Deluxe: quick save triggered from submenu (244B).
 * Initiates a background save without requiring the NPC restart dialog.
 * Polls mCD_SaveHome_bg each frame until save completes or errors. */
extern void mSM_quick_save(GAME_PLAY* play) {
    static int quick_save_state = 0;
    static int quick_save_chan = -1;
    int result;

    switch (quick_save_state) {
        case 0:
            /* Check if save is appropriate */
            if (mEv_CheckFirstJob() || mEv_CheckFirstIntro()) {
                quick_save_state = 0;
                return;
            }

            if (mDemo_CheckDemo()) {
                quick_save_state = 0;
                return;
            }

            /* Begin save (param 1 = continue playing, don't go to title) */
            quick_save_state = 1;
            quick_save_chan = -1;
            break;

        case 1:
            /* Poll the save background task */
            result = mCD_SaveHome_bg(1, &quick_save_chan);

            switch (result) {
                case mCD_TRANS_ERR_NONE:
                    /* Save completed successfully */
                    quick_save_state = 0;
                    break;
                case mCD_TRANS_ERR_BUSY:
                    /* Still in progress */
                    break;
                default:
                    /* Error occurred - abort */
                    quick_save_state = 0;
                    break;
            }
            break;

        default:
            quick_save_state = 0;
            break;
    }
}

#if VERSION >= VER_DELUXE
/* Deluxe: restore door data from structure exit data (60B).
 * Called when returning from submenu to restore the door transition state. */
extern void mSM_submenu_restore_door_data(void) {
    Common_Get(door_data) = Common_Get(structure_exit_door_data);
}

/* Deluxe: initialize extended submenu members (136B).
 * Reinitializes submenu state fields without doing a full bzero,
 * used during save load and new game initialization. */
extern void mSM_extMemberInit(Submenu* submenu) {
    int i;
    Submenu_Item_c* item;

    submenu->mode = mSM_MODE_IDLE;
    submenu->menu_type = mSM_OVL_NONE;
    submenu->current_menu_type = mSM_OVL_NONE;
    submenu->process_status = mSM_PROCESS_WAIT;
    submenu->wait_timer = 0;
    submenu->open_flag = FALSE;
    submenu->after_mode = 0;
    submenu->unk_164 = 0;
    submenu->start_refuse = FALSE;
    submenu->start_refuse_timer = 0;
    submenu->move_proc = (SUBMENU_PROC)&none_proc1;
    submenu->draw_proc = (SUBMENU_GAME_PROC)&none_proc1;
    submenu->item_p = &submenu->items[0];
    submenu->item_num = mPr_POCKETS_SLOT_COUNT;
    submenu->selected_item_num = 0;

    item = submenu->item_p;
    for (i = 0; i < mPr_POCKETS_SLOT_COUNT; i++) {
        item->item = EMPTY_NO;
        item->slot_no = mPr_POCKETS_SLOT_COUNT;
        item++;
    }
}

/* Deluxe: check if pocket item is a fossil for museum curator (64B).
 * Used when donating items to the museum's fossil wing. */
static int mSM_check_item_for_curator_fossil(int slot_no, int param_2) {
    Private_c* priv = Common_Get(now_private);
    mActor_name_t item = priv->inventory.pockets[slot_no];

    if (item != EMPTY_NO &&
        mPr_GET_ITEM_COND(priv->inventory.item_conditions, slot_no) == mPr_ITEM_COND_NORMAL &&
        item == ITM_FOSSIL) {
        return TRUE;
    }

    return FALSE;
}

/* Deluxe: check if pocket item can be placed in storage (44B).
 * Used when interacting with dresser/closet storage furniture. */
static int mSM_check_item_for_storage(int slot_no, int param_2) {
    Private_c* priv = Common_Get(now_private);
    mActor_name_t item = priv->inventory.pockets[slot_no];

    if (item != EMPTY_NO &&
        mPr_GET_ITEM_COND(priv->inventory.item_conditions, slot_no) == mPr_ITEM_COND_NORMAL) {
        return TRUE;
    }

    return FALSE;
}
#endif
