#include "m_name_table.h"

#include "m_actor.h"
#include "m_npc_personal_id.h"
#include "m_common_data.h"

/* Forward declarations for Deluxe functions */
extern int mLd_PlayerManKindCheck();
extern u32 mPr_GetPossessionItemSumWithCond(Private_c* priv, mActor_name_t item, u32 cond);

#if VERSION >= VER_DELUXE
#include "m_item_info.h"
#include "m_malloc.h"
#include "dolphin/dvd.h"

/* Deluxe: implementations provided by m_item_info.c */
extern int mIF_Item2Idx(mActor_name_t item);
extern u8* mIF_GetInfo(int idx);
extern u8* mIF_GetInfoForItem(mActor_name_t item);
#else
extern u8* mIF_GetInfoForItem(mActor_name_t item);
#endif

s16 move_obj_profile_table[] = {
    mAc_PROFILE_AIRPLANE,        mAc_PROFILE_BALL,         mAc_PROFILE_MY_ROOM,      mAc_PROFILE_MBG,
    mAc_PROFILE_BOXMANAGER,      mAc_PROFILE_BOXTRICK01,   mAc_PROFILE_ARRANGE_ROOM, mAc_PROFILE_ARRANGE_FURNITURE,
    mAc_PROFILE_EFFECT_CONTROL,  mAc_PROFILE_SHOP_DESIGN,  mAc_PROFILE_SHOP_MANEKIN, mAc_PROFILE_SHOP_INDOOR,
    mAc_PROFILE_SHOP_GOODS,      mAc_PROFILE_SNOWMAN,      mAc_PROFILE_SNOWMAN,      mAc_PROFILE_BROKER_DESIGN,
    mAc_PROFILE_MY_INDOOR,       mAc_PROFILE_TRAIN_WINDOW, mAc_PROFILE_UKI,          mAc_PROFILE_SHOP_UMBRELLA,
    mAc_PROFILE_KAMAKURA_INDOOR, mAc_PROFILE_HOUSE_CLOCK
};

s16 actor_profile_table[] = { mAc_PROFILE_SAMPLE };

s16 props_profile_table[] = {
    mAc_PROFILE_MAILBOX,  mAc_PROFILE_MAILBOX,  mAc_PROFILE_MAILBOX,   mAc_PROFILE_MAILBOX,   mAc_PROFILE_HANIWA,
    mAc_PROFILE_HANIWA,   mAc_PROFILE_HANIWA,   mAc_PROFILE_HANIWA,    mAc_PROFILE_PSNOWMAN,  mAc_PROFILE_PSNOWMAN,
    mAc_PROFILE_PSNOWMAN, mAc_PROFILE_PSNOWMAN, mAc_PROFILE_PSNOWMAN,  mAc_PROFILE_PSNOWMAN,  mAc_PROFILE_PSNOWMAN,
    mAc_PROFILE_PSNOWMAN, mAc_PROFILE_PSNOWMAN, mAc_PROFILE_TRAINDOOR, mAc_PROFILE_NAMEPLATE,
};

u8 npc_looks_table[NPC_NUM + 2] = {
    mNpc_LOOKS_BOY,         mNpc_LOOKS_NANIWA_LADY, mNpc_LOOKS_GIRL,        mNpc_LOOKS_GIRL,
    mNpc_LOOKS_KO_GIRL,     mNpc_LOOKS_GRIM_MAN,    mNpc_LOOKS_KO_GIRL,     mNpc_LOOKS_NANIWA_LADY,
    mNpc_LOOKS_SPORT_MAN,   mNpc_LOOKS_NANIWA_LADY, mNpc_LOOKS_NANIWA_LADY, mNpc_LOOKS_GRIM_MAN,
    mNpc_LOOKS_KO_GIRL,     mNpc_LOOKS_GIRL,        mNpc_LOOKS_GIRL,        mNpc_LOOKS_KO_GIRL,
    mNpc_LOOKS_GRIM_MAN,    mNpc_LOOKS_NANIWA_LADY, mNpc_LOOKS_SPORT_MAN,   mNpc_LOOKS_GRIM_MAN,
    mNpc_LOOKS_BOY,         mNpc_LOOKS_BOY,         mNpc_LOOKS_GRIM_MAN,    mNpc_LOOKS_KO_GIRL,
    mNpc_LOOKS_NANIWA_LADY, mNpc_LOOKS_GRIM_MAN,    mNpc_LOOKS_KO_GIRL,     mNpc_LOOKS_SPORT_MAN,
    mNpc_LOOKS_KO_GIRL,     mNpc_LOOKS_BOY,         mNpc_LOOKS_KO_GIRL,     mNpc_LOOKS_GRIM_MAN,
    mNpc_LOOKS_BOY,         mNpc_LOOKS_NANIWA_LADY, mNpc_LOOKS_GIRL,        mNpc_LOOKS_SPORT_MAN,
    mNpc_LOOKS_GRIM_MAN,    mNpc_LOOKS_GRIM_MAN,    mNpc_LOOKS_BOY,         mNpc_LOOKS_SPORT_MAN,
    mNpc_LOOKS_GIRL,        mNpc_LOOKS_SPORT_MAN,   mNpc_LOOKS_BOY,         mNpc_LOOKS_GIRL,
    mNpc_LOOKS_SPORT_MAN,   mNpc_LOOKS_KO_GIRL,     mNpc_LOOKS_NANIWA_LADY, mNpc_LOOKS_GRIM_MAN,
    mNpc_LOOKS_GRIM_MAN,    mNpc_LOOKS_NANIWA_LADY, mNpc_LOOKS_SPORT_MAN,   mNpc_LOOKS_GIRL,
    mNpc_LOOKS_BOY,         mNpc_LOOKS_GIRL,        mNpc_LOOKS_SPORT_MAN,   mNpc_LOOKS_KO_GIRL,
    mNpc_LOOKS_KO_GIRL,     mNpc_LOOKS_NANIWA_LADY, mNpc_LOOKS_GIRL,        mNpc_LOOKS_KO_GIRL,
    mNpc_LOOKS_BOY,         mNpc_LOOKS_SPORT_MAN,   mNpc_LOOKS_GIRL,        mNpc_LOOKS_BOY,
    mNpc_LOOKS_GIRL,        mNpc_LOOKS_GIRL,        mNpc_LOOKS_BOY,         mNpc_LOOKS_NANIWA_LADY,
    mNpc_LOOKS_KO_GIRL,     mNpc_LOOKS_GRIM_MAN,    mNpc_LOOKS_SPORT_MAN,   mNpc_LOOKS_KO_GIRL,
    mNpc_LOOKS_BOY,         mNpc_LOOKS_SPORT_MAN,   mNpc_LOOKS_BOY,         mNpc_LOOKS_KO_GIRL,
    mNpc_LOOKS_BOY,         mNpc_LOOKS_BOY,         mNpc_LOOKS_SPORT_MAN,   mNpc_LOOKS_GIRL,
    mNpc_LOOKS_SPORT_MAN,   mNpc_LOOKS_GRIM_MAN,    mNpc_LOOKS_SPORT_MAN,   mNpc_LOOKS_KO_GIRL,
    mNpc_LOOKS_GIRL,        mNpc_LOOKS_BOY,         mNpc_LOOKS_BOY,         mNpc_LOOKS_GRIM_MAN,
    mNpc_LOOKS_SPORT_MAN,   mNpc_LOOKS_BOY,         mNpc_LOOKS_GIRL,        mNpc_LOOKS_SPORT_MAN,
    mNpc_LOOKS_GIRL,        mNpc_LOOKS_SPORT_MAN,   mNpc_LOOKS_GRIM_MAN,    mNpc_LOOKS_BOY,
    mNpc_LOOKS_NANIWA_LADY, mNpc_LOOKS_NANIWA_LADY, mNpc_LOOKS_GRIM_MAN,    mNpc_LOOKS_SPORT_MAN,
    mNpc_LOOKS_GRIM_MAN,    mNpc_LOOKS_BOY,         mNpc_LOOKS_KO_GIRL,     mNpc_LOOKS_GIRL,
    mNpc_LOOKS_BOY,         mNpc_LOOKS_KO_GIRL,     mNpc_LOOKS_GIRL,        mNpc_LOOKS_SPORT_MAN,
    mNpc_LOOKS_NANIWA_LADY, mNpc_LOOKS_GIRL,        mNpc_LOOKS_SPORT_MAN,   mNpc_LOOKS_GIRL,
    mNpc_LOOKS_NANIWA_LADY, mNpc_LOOKS_KO_GIRL,     mNpc_LOOKS_GIRL,        mNpc_LOOKS_NANIWA_LADY,
    mNpc_LOOKS_NANIWA_LADY, mNpc_LOOKS_GIRL,        mNpc_LOOKS_GRIM_MAN,    mNpc_LOOKS_BOY,
    mNpc_LOOKS_NANIWA_LADY, mNpc_LOOKS_KO_GIRL,     mNpc_LOOKS_BOY,         mNpc_LOOKS_SPORT_MAN,
    mNpc_LOOKS_SPORT_MAN,   mNpc_LOOKS_SPORT_MAN,   mNpc_LOOKS_KO_GIRL,     mNpc_LOOKS_GIRL,
    mNpc_LOOKS_BOY,         mNpc_LOOKS_KO_GIRL,     mNpc_LOOKS_GRIM_MAN,    mNpc_LOOKS_KO_GIRL,
    mNpc_LOOKS_GRIM_MAN,    mNpc_LOOKS_NANIWA_LADY, mNpc_LOOKS_GRIM_MAN,    mNpc_LOOKS_GIRL,
    mNpc_LOOKS_SPORT_MAN,   mNpc_LOOKS_KO_GIRL,     mNpc_LOOKS_NANIWA_LADY, mNpc_LOOKS_GRIM_MAN,
    mNpc_LOOKS_GRIM_MAN,    mNpc_LOOKS_SPORT_MAN,   mNpc_LOOKS_NANIWA_LADY, mNpc_LOOKS_SPORT_MAN,
    mNpc_LOOKS_SPORT_MAN,   mNpc_LOOKS_GIRL,        mNpc_LOOKS_GRIM_MAN,    mNpc_LOOKS_BOY,
    mNpc_LOOKS_BOY,         mNpc_LOOKS_NANIWA_LADY, mNpc_LOOKS_NANIWA_LADY, mNpc_LOOKS_SPORT_MAN,
    mNpc_LOOKS_KO_GIRL,     mNpc_LOOKS_BOY,         mNpc_LOOKS_BOY,         mNpc_LOOKS_GRIM_MAN,
    mNpc_LOOKS_NANIWA_LADY, mNpc_LOOKS_SPORT_MAN,   mNpc_LOOKS_GIRL,        mNpc_LOOKS_SPORT_MAN,
    mNpc_LOOKS_GRIM_MAN,    mNpc_LOOKS_KO_GIRL,     mNpc_LOOKS_BOY,         mNpc_LOOKS_GRIM_MAN,
    mNpc_LOOKS_GIRL,        mNpc_LOOKS_KO_GIRL,     mNpc_LOOKS_KO_GIRL,     mNpc_LOOKS_SPORT_MAN,
    mNpc_LOOKS_SPORT_MAN,   mNpc_LOOKS_BOY,         mNpc_LOOKS_SPORT_MAN,   mNpc_LOOKS_GRIM_MAN,
    mNpc_LOOKS_NANIWA_LADY, mNpc_LOOKS_GIRL,        mNpc_LOOKS_BOY,         mNpc_LOOKS_GIRL,
    mNpc_LOOKS_KO_GIRL,     mNpc_LOOKS_NANIWA_LADY, mNpc_LOOKS_GRIM_MAN,    mNpc_LOOKS_GRIM_MAN,
    mNpc_LOOKS_NANIWA_LADY, mNpc_LOOKS_KO_GIRL,     mNpc_LOOKS_SPORT_MAN,   mNpc_LOOKS_GRIM_MAN,
    mNpc_LOOKS_KO_GIRL,     mNpc_LOOKS_GRIM_MAN,    mNpc_LOOKS_NANIWA_LADY, mNpc_LOOKS_GRIM_MAN,
    mNpc_LOOKS_BOY,         mNpc_LOOKS_NANIWA_LADY, mNpc_LOOKS_GRIM_MAN,    mNpc_LOOKS_BOY,
    mNpc_LOOKS_GRIM_MAN,    mNpc_LOOKS_BOY,         mNpc_LOOKS_SPORT_MAN,   mNpc_LOOKS_NANIWA_LADY,
    mNpc_LOOKS_BOY,         mNpc_LOOKS_GIRL,        mNpc_LOOKS_NANIWA_LADY, mNpc_LOOKS_GRIM_MAN,
    mNpc_LOOKS_KO_GIRL,     mNpc_LOOKS_NANIWA_LADY, mNpc_LOOKS_GIRL,        mNpc_LOOKS_GIRL,
    mNpc_LOOKS_NANIWA_LADY, mNpc_LOOKS_NANIWA_LADY, mNpc_LOOKS_KO_GIRL,     mNpc_LOOKS_GIRL,
    mNpc_LOOKS_NANIWA_LADY, mNpc_LOOKS_GRIM_MAN,    mNpc_LOOKS_GIRL,        mNpc_LOOKS_KO_GIRL,
    mNpc_LOOKS_GIRL,        mNpc_LOOKS_NANIWA_LADY, mNpc_LOOKS_SPORT_MAN,   mNpc_LOOKS_KO_GIRL,
    mNpc_LOOKS_NANIWA_LADY, mNpc_LOOKS_GRIM_MAN,    mNpc_LOOKS_GIRL,        mNpc_LOOKS_SPORT_MAN,
    mNpc_LOOKS_SPORT_MAN,   mNpc_LOOKS_BOY,         mNpc_LOOKS_KO_GIRL,     mNpc_LOOKS_GRIM_MAN,
    mNpc_LOOKS_BOY,         mNpc_LOOKS_KO_GIRL,     mNpc_LOOKS_NANIWA_LADY, mNpc_LOOKS_KO_GIRL,
    mNpc_LOOKS_GIRL,        mNpc_LOOKS_BOY,         mNpc_LOOKS_GRIM_MAN,    mNpc_LOOKS_GIRL,
    mNpc_LOOKS_KO_GIRL,     mNpc_LOOKS_SPORT_MAN,   mNpc_LOOKS_NANIWA_LADY, mNpc_LOOKS_BOY,
    mNpc_LOOKS_BOY,         mNpc_LOOKS_KO_GIRL
};

static u8 item1_0_tableNo[] = {
    44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44,
    44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44,
    44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44,
    44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44,
    44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44,
    44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44,
    44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44,
    44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44,
    44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44,
};

static u8 item1_1_tableNo[] = { 9, 9, 9, 9 };

static u8 item1_2_tableNo[] = {
    34, 33, 36, 35, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41,
    41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 19, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 38, 37, 40, 39, 33, 33, 33, 33, 33, 33, 33, 19,
    19, 19, 19, 19, 19, 19, 19, 42, 42, 42, 42, 42, 42, 42, 42, 43, 43, 43, 43, 43, 43, 43, 43,
};

static u8 item1_3_tableNo[] = {
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
};

static u8 item1_4_tableNo[] = {
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
};

static u8 item1_5_tableNo[] = {
    22, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 9,  21, 21, 21, 20, 23, 29, 25, 24, 24, 24, 25,
    25, 24, 26, 14, 11, 19, 14, 14, 14, 14, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 52,
};

static u8 item1_6_tableNo[] = {
    31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
    31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
    31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
};

static u8 item1_7_tableNo[] = {
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
};

static u8 item1_8_tableNo[] = { 1, 5, 4, 3, 2, 6, 27, 28 };

static u8 item1_9_tableNo[] = { 17, 17, 45, 45, 45, 45, 45, 45, 45, 45, 45 };

static u8 item1_A_tableNo[] = {
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
};

// ?? Shouldn't these be different from saplings?
static u8 item1_B_tableNo[] = { 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17 };

static u8 item1_C_tableNo[] = {
    49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49,
    49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49,
    49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49,
    49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49,
};

static u8 item1_D_tableNo[] = {
    18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
    18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
};

static u8 item1_E_tableNo[] = { 46, 46 };

static u8 item1_F_tableNo[] = { 7, 7, 7, 7 };

extern int mNT_get_itemTableNo(mActor_name_t item) {
    static u8* item1_tableNo[ITEM1_CAT_NUM] = { item1_0_tableNo, item1_1_tableNo, item1_2_tableNo, item1_3_tableNo,
                                                item1_4_tableNo, item1_5_tableNo, item1_6_tableNo, item1_7_tableNo,
                                                item1_8_tableNo, item1_9_tableNo, item1_A_tableNo, item1_B_tableNo,
                                                item1_C_tableNo, item1_D_tableNo, item1_E_tableNo, item1_F_tableNo };

    switch (ITEM_NAME_GET_TYPE(item)) {
        case NAME_TYPE_FTR0:
        case NAME_TYPE_FTR1: {
            if (item >= FTR_DINO_START && item <= FTR_DINO_END) {
                return mNT_ITEM_TYPE_BONE;
            } else if (item >= HANIWA_START && item <= HANIWA_END) {
                return mNT_ITEM_TYPE_HANIWA;
            } else if (item >= ITM_DIARY_START && item <= ITM_DIARY15) {
                return mNT_ITEM_TYPE_DIARY;
            }

            return mNT_ITEM_TYPE_LEAF;
        }

        case NAME_TYPE_ITEM1: {
            int category = ITEM_NAME_GET_CAT((mActor_name_t)item);
            int idx = ITEM_NAME_GET_INDEX((mActor_name_t)item);

            return item1_tableNo[category][idx];
        }

        default:
            return mNT_ITEM_TYPE_NONE;
    }
}

extern mActor_name_t mNT_FishIdx2FishItemNo(int idx) {
    if (idx >= 0 && idx <= (FISH_NUM + 1)) {
        return ITM_FISH_START + idx;
    }

    return EMPTY_NO;
}

extern mActor_name_t bg_item_fg_sub(mActor_name_t item, s16 flag) {
    static mActor_name_t cnvfg[mNT_TREE_SIZE_NUM][mNT_TREE_TYPE_NUM] = {
        { TREE_STUMP004, TREE_PALM_STUMP004, CEDAR_TREE_STUMP004, GOLD_TREE_STUMP004 },
        { TREE_STUMP003, TREE_PALM_STUMP003, CEDAR_TREE_STUMP003, GOLD_TREE_STUMP003 },
        { TREE_STUMP002, TREE_PALM_STUMP002, CEDAR_TREE_STUMP002, GOLD_TREE_STUMP002 },
        { TREE_STUMP001, TREE_PALM_STUMP001, CEDAR_TREE_STUMP001, GOLD_TREE_STUMP001 }
    };

    u32 what = item;
    int tree_type = FGTreeType_check(what);
    mActor_name_t stump = what;

    if (flag != 0) {
        stump = what;
    } else if (item == TREE || item == TREE_1000BELLS || item == TREE_10000BELLS || item == TREE_30000BELLS ||
               item == TREE_100BELLS || item == CEDAR_TREE || item == GOLD_TREE_SHOVEL || item == GOLD_TREE ||
               item == TREE_APPLE_NOFRUIT_0 || item == TREE_APPLE_NOFRUIT_1 || item == TREE_APPLE_NOFRUIT_2 ||
               item == TREE_APPLE_FRUIT || item == TREE_ORANGE_NOFRUIT_0 || item == TREE_ORANGE_NOFRUIT_1 ||
               item == TREE_ORANGE_NOFRUIT_2 || item == TREE_ORANGE_FRUIT || item == TREE_PEACH_NOFRUIT_0 ||
               item == TREE_PEACH_NOFRUIT_1 || item == TREE_PEACH_NOFRUIT_2 || item == TREE_PEACH_FRUIT ||
               item == TREE_PEAR_NOFRUIT_0 || item == TREE_PEAR_NOFRUIT_1 || item == TREE_PEAR_NOFRUIT_2 ||
               item == TREE_PEAR_FRUIT || item == TREE_CHERRY_NOFRUIT_0 || item == TREE_CHERRY_NOFRUIT_1 ||
               item == TREE_CHERRY_NOFRUIT_2 || item == TREE_CHERRY_FRUIT || item == TREE_PALM_NOFRUIT_0 ||
               item == TREE_PALM_NOFRUIT_1 || item == TREE_PALM_NOFRUIT_2 || item == TREE_PALM_FRUIT ||
               item == TREE_BEES || item == TREE_FTR || item == TREE_LIGHTS || item == TREE_PRESENT ||
               item == TREE_BELLS || item == CEDAR_TREE_BELLS || item == CEDAR_TREE_FTR || item == CEDAR_TREE_BEES ||
               item == CEDAR_TREE_LIGHTS || item == GOLD_TREE_BELLS || item == GOLD_TREE_FTR ||
               item == GOLD_TREE_BEES) {
        stump = cnvfg[mNT_TREE_SIZE_FULL][tree_type];
    } else if (item == TREE_S2 || item == TREE_APPLE_S2 || item == TREE_ORANGE_S2 || item == TREE_PEACH_S2 ||
               item == TREE_PEAR_S2 || item == TREE_CHERRY_S2 || item == TREE_1000BELLS_S2 ||
               item == TREE_10000BELLS_S2 || item == TREE_30000BELLS_S2 || item == TREE_100BELLS_S2 ||
               item == TREE_PALM_S2 || item == CEDAR_TREE_S2 || item == GOLD_TREE_S2) {
        stump = cnvfg[mNT_TREE_SIZE_S2][tree_type];
    } else if (item == TREE_S1 || item == TREE_APPLE_S1 || item == TREE_ORANGE_S1 || item == TREE_PEACH_S1 ||
               item == TREE_PEAR_S1 || item == TREE_CHERRY_S1 || item == TREE_1000BELLS_S1 ||
               item == TREE_10000BELLS_S1 || item == TREE_30000BELLS_S1 || item == TREE_100BELLS_S1 ||
               item == TREE_PALM_S1 || item == CEDAR_TREE_S1 || item == GOLD_TREE_S1) {
        stump = cnvfg[mNT_TREE_SIZE_S1][tree_type];
    } else if (item == TREE_S0 || item == TREE_APPLE_S0 || item == TREE_ORANGE_S0 || item == TREE_PEACH_S0 ||
               item == TREE_PEAR_S0 || item == TREE_CHERRY_S0 || item == TREE_1000BELLS_S0 ||
               item == TREE_10000BELLS_S0 || item == TREE_30000BELLS_S0 || item == TREE_100BELLS_S0 ||
               item == TREE_PALM_S0 || item == CEDAR_TREE_S0 || item == GOLD_TREE_S0) {
        stump = cnvfg[mNT_TREE_SIZE_S0][tree_type];
    }

    return stump;
}

extern mActor_name_t bg_item_fg_sub_tree_grow(mActor_name_t item, int past_days, int check_plant) {
    static s16 grow_check[84][2] = {
        { -1, mCoBG_PLANT0 }, { -1, mCoBG_PLANT1 }, { -1, mCoBG_PLANT2 }, { -1, mCoBG_PLANT3 }, { 0, mCoBG_PLANT4 },
        { -1, mCoBG_PLANT0 }, { -1, mCoBG_PLANT1 }, { -1, mCoBG_PLANT2 }, { -4, mCoBG_PLANT3 }, { -1, mCoBG_PLANT4 },
        { -1, mCoBG_PLANT4 }, { -1, mCoBG_PLANT4 }, { 0, mCoBG_PLANT4 },  { -1, mCoBG_PLANT0 }, { -1, mCoBG_PLANT1 },
        { -1, mCoBG_PLANT2 }, { -4, mCoBG_PLANT3 }, { -1, mCoBG_PLANT4 }, { -1, mCoBG_PLANT4 }, { -1, mCoBG_PLANT4 },
        { 0, mCoBG_PLANT4 },  { -1, mCoBG_PLANT0 }, { -1, mCoBG_PLANT1 }, { -1, mCoBG_PLANT2 }, { -4, mCoBG_PLANT3 },
        { -1, mCoBG_PLANT4 }, { -1, mCoBG_PLANT4 }, { -1, mCoBG_PLANT4 }, { 0, mCoBG_PLANT4 },  { -1, mCoBG_PLANT0 },
        { -1, mCoBG_PLANT1 }, { -1, mCoBG_PLANT2 }, { -4, mCoBG_PLANT3 }, { -1, mCoBG_PLANT4 }, { -1, mCoBG_PLANT4 },
        { -1, mCoBG_PLANT4 }, { 0, mCoBG_PLANT4 },  { -1, mCoBG_PLANT0 }, { -1, mCoBG_PLANT1 }, { -1, mCoBG_PLANT2 },
        { -4, mCoBG_PLANT3 }, { -1, mCoBG_PLANT4 }, { -1, mCoBG_PLANT4 }, { -1, mCoBG_PLANT4 }, { 0, mCoBG_PLANT4 },
        { -1, mCoBG_PLANT0 }, { -1, mCoBG_PLANT1 }, { -1, mCoBG_PLANT2 }, { -1, mCoBG_PLANT3 }, { 0, mCoBG_PLANT4 },
        { -1, mCoBG_PLANT0 }, { -1, mCoBG_PLANT1 }, { -1, mCoBG_PLANT2 }, { -1, mCoBG_PLANT3 }, { 0, mCoBG_PLANT4 },
        { -1, mCoBG_PLANT0 }, { -1, mCoBG_PLANT1 }, { -1, mCoBG_PLANT2 }, { -1, mCoBG_PLANT3 }, { 0, mCoBG_PLANT4 },
        { -1, mCoBG_PLANT0 }, { -1, mCoBG_PLANT1 }, { -1, mCoBG_PLANT2 }, { -1, mCoBG_PLANT3 }, { 0, mCoBG_PLANT4 },
        { -1, mCoBG_PLANT0 }, { -1, mCoBG_PLANT1 }, { -1, mCoBG_PLANT2 }, { -4, mCoBG_PLANT3 }, { -1, mCoBG_PLANT4 },
        { -1, mCoBG_PLANT4 }, { -1, mCoBG_PLANT4 }, { 0, mCoBG_PLANT4 },  { -1, mCoBG_PLANT0 }, { -1, mCoBG_PLANT1 },
        { -1, mCoBG_PLANT2 }, { -1, mCoBG_PLANT3 }, { 0, mCoBG_PLANT4 },  { -1, mCoBG_PLANT0 }, { -1, mCoBG_PLANT1 },
        { -1, mCoBG_PLANT2 }, { -1, mCoBG_PLANT3 }, { 0, mCoBG_PLANT4 },  { 0, mCoBG_PLANT4 }
    };

    int idx;
    int i;

    if ((item >= TREE_SAPLING && item <= TREE_30000BELLS) ||
        (item >= TREE_100BELLS_SAPLING && item <= TREE_PALM_FRUIT) ||
        (item >= CEDAR_TREE_SAPLING && item <= CEDAR_TREE) || (item >= GOLD_TREE_SAPLING && item <= GOLD_TREE)) {
        if (item >= TREE_PALM_SAPLING && item <= TREE_PALM_FRUIT) {
            idx = item - 0x0813;
        } else if (item >= CEDAR_TREE_SAPLING && item <= CEDAR_TREE) {
            idx = item - 0x0814;
        } else if (item == GOLD_TREE || (item >= GOLD_TREE_SAPLING && item <= GOLD_TREE_SHOVEL)) {
            idx = item - 0x0815;
        } else if (item >= TREE_100BELLS_SAPLING) {
            idx = item - 0x0813;
        } else {
            idx = item - 0x0800;
        }

        for (i = 0; i <= past_days; i++) {
            int diff = -grow_check[idx][0];

            if (grow_check[idx + diff][1] > check_plant) {
                break;
            }

            idx += diff;
            item += diff;
        }
    }

    return item;
}

extern mActor_name_t bg_item_fg_sub_dig2take_conv(mActor_name_t item) {
    mActor_name_t dig_item;

    dig_item = item;
    if ((item >= BURIED_PITFALL_HOLE_START) && (item <= BURIED_PITFALL_HOLE_END)) {
        dig_item = ITM_PITFALL;
    }

    if (item == SHINE_SPOT) {
        f32 rng_val = RANDOM_F(100.0f);
        f32 money_power = mPr_GetMoneyPower();
        f32 max_bells_roll = 2.0f * (money_power / 40.0f);    // 30k
        f32 large_bells_roll = 10.0f * (money_power / 40.0f); // 10k

        if ((rng_val <= max_bells_roll) || ((int)Common_Get(now_private)->destiny.type == mPr_DESTINY_MONEY_LUCK)) {
            dig_item = ITM_MONEY_30000;
        } else if (rng_val <= (large_bells_roll + max_bells_roll)) {
            dig_item = ITM_MONEY_10000;
        } else {
            dig_item = ITM_MONEY_1000;
        }
    }
    return dig_item;
}

static mNT_offset_table_c height_table[3] = {
    { 0, { mCoBG_ATTRIBUTE_NONE, 0, 0, 0, 0, 0, 0 } },
    { 0, { mCoBG_ATTRIBUTE_WOOD, 4, 4, 4, 4, 4, 0 } },
    { 0, { mCoBG_ATTRIBUTE_WOOD, 7, 7, 7, 7, 7, 0 } },
};

extern mNT_offset_table_c* obj_hight_table_item0_nogrow(mActor_name_t item) {
    mNT_offset_table_c* table = &height_table[0];

    if (item == FENCE0 || item == FENCE1 || item == MESSAGE_BOARD0 || item == MESSAGE_BOARD1 || item == WOOD_FENCE) {
        table = &height_table[1];
    } else if (item == MAP_BOARD0 || item == MAP_BOARD1 || item == MUSIC_BOARD0 || item == MUSIC_BOARD1) {
        table = &height_table[2];
    }

    return table;
}

extern int FGTreeType_check(mActor_name_t tree) {
    if ((tree >= TREE_PALM_SAPLING && tree <= TREE_PALM_FRUIT) || (tree == DEAD_PALM_SAPLING) ||
        (tree >= TREE_PALM_STUMP001 && tree <= TREE_PALM_STUMP004)) {
        return mNT_TREE_TYPE_PALM;
    }

    if ((tree >= CEDAR_TREE_SAPLING && tree <= CEDAR_TREE) || (tree == DEAD_CEDAR_SAPLING) ||
        (tree == CEDAR_TREE_BELLS) || (tree == CEDAR_TREE_FTR) || (tree == CEDAR_TREE_BEES) ||
        (tree == CEDAR_TREE_LIGHTS) || (tree >= CEDAR_TREE_STUMP001 && tree <= CEDAR_TREE_STUMP004)) {
        return mNT_TREE_TYPE_CEDAR;
    }

    if ((tree == GOLD_TREE) || (tree >= GOLD_TREE_SAPLING && tree <= GOLD_TREE_SHOVEL) || (tree == DEAD_GOLD_SAPLING) ||
        (tree == GOLD_TREE_BELLS) || (tree == GOLD_TREE_FTR) || (tree == GOLD_TREE_BEES) ||
        (tree >= GOLD_TREE_STUMP001 && tree <= GOLD_TREE_STUMP004)) {
        return mNT_TREE_TYPE_GOLD;
    }

    return mNT_TREE_TYPE_NORMAL;
}

extern int mNT_ItIsStump(mActor_name_t actor) {
    if ((actor >= TREE_STUMP001 && actor <= TREE_STUMP004) ||
        (actor >= TREE_PALM_STUMP001 && actor <= TREE_PALM_STUMP004) ||
        (actor >= CEDAR_TREE_STUMP001 && actor <= CEDAR_TREE_STUMP004) ||
        (actor >= GOLD_TREE_STUMP001 && actor <= GOLD_TREE_STUMP004)) {
        return TRUE;
    }

    return FALSE;
}

extern int mNT_ItIsStoneCoin10(mActor_name_t actor) {
    int res = FALSE;

    if ((actor >= MONEY_ROCK_A) && (actor <= MONEY_FLOWER_SEED)) {
        res = TRUE;
    }

    return res != FALSE;
}

extern int mNT_ItIsReserveDummy(mActor_name_t actor) {
    return actor == DUMMY_RESERVE;
}

extern int mNT_check_unknown(mActor_name_t item_no) {
    static s16 item1_kinds[ITEM1_CAT_NUM] = {
        PAPER_NUM, MONEY_NUM, TOOL_NUM,     FISH_NUM,  CLOTH_NUM,  ETC_NUM,    CARPET_NUM,     WALL_NUM,
        FRUIT_NUM, PLANT_NUM, MINIDISK_NUM, DIARY_NUM, TICKET_NUM, INSECT_NUM, HUKUBUKURO_NUM, KABU_NUM,
    };

    int index = ITEM_NAME_GET_INDEX(item_no);
    int res = FALSE;

    switch (ITEM_NAME_GET_TYPE(item_no)) {
        case NAME_TYPE_ITEM1: {
            int category = ITEM_NAME_GET_CAT(item_no);

            if (index < 0 || index >= item1_kinds[category]) {
                res = TRUE;
            }
            break;
        }

        case NAME_TYPE_FTR1: {
            int FtrIdx = mRmTp_FtrItemNo2FtrIdx(item_no);

            if (FtrIdx < 0 || FtrIdx >= FTR_NUM) {
                res = TRUE;
            }

            break;
        }
    }

    return res;
}

/* Deluxe: convert item1 flower item to item0 flower */
static mActor_name_t dlx_flower_item1_to_item0[] = {
    0x0845, 0x0846, 0x0847, 0x086E, 0x086F,
    0x0870, 0x0848, 0x0849, 0x084A, 0x0871,
    0x0872, 0x0873, 0x0874, 0x084B, 0x084C,
    0x084D, 0x086A, 0x086B, 0x086C, 0x086D,
    0x0875, 0x0888, 0x0889, 0x088A, 0x088B,
    0x088C, 0x088D, 0x088E, 0x088F, 0x0890,
    0x0891,
};

/* Deluxe: convert flower bag to item0 flower */
static mActor_name_t dlx_flowerbag_to_item0[] = {
    0x0845, 0x086E, 0x0847, 0x0848, 0x0872,
    0x0871, 0x084B, 0x084C, 0x084D,
};

extern mActor_name_t item1_flower_2_item0_flower(mActor_name_t item) {
    if ((u16)item >= ITM_DLX_FLOWER_START && (u16)item <= ITM_DLX_FLOWER_END) {
        return dlx_flower_item1_to_item0[(u16)item - ITM_DLX_FLOWER_START];
    }

    return 0;
}

extern mActor_name_t mNT_flowerbag2flower(mActor_name_t item) {
    if ((u16)item >= ITM_WHITE_PANSY_BAG && (u16)item <= ITM_YELLOW_TULIP_BAG) {
        return dlx_flowerbag_to_item0[(u16)item - ITM_WHITE_PANSY_BAG];
    }

    if ((u16)item >= ITM_DLX_FLOWERBAG_START && (u16)item <= ITM_DLX_FLOWERBAG_END) {
        return (mActor_name_t)((u16)item - 0x20A2);
    }

    return item;
}

extern int mNT_ItIsFlower(mActor_name_t item) {
    u8 res = FALSE;

    if ((u16)item >= FLOWER_LEAVES_PANSIES0 && (u16)item <= FLOWER_TULIP2) {
        res = TRUE;
    } else if ((u16)item >= DLX_FLOWER_RANGE1_START && (u16)item <= DLX_FLOWER_RANGE1_END) {
        res = TRUE;
    } else if ((u16)item >= DLX_FLOWER_RANGE2_START && (u16)item <= DLX_FLOWER_RANGE2_END) {
        res = TRUE;
    }

    return (u8)res;
}

extern int mNT_ItIsFlowerSeed(mActor_name_t item) {
    u8 res = FALSE;

    if ((u16)item >= FLOWER_LEAVES_PANSIES0 && (u16)item <= DLX_FLOWER_SEED_END) {
        res = TRUE;
    } else if ((u16)item >= FLOWER_LEAVES_PANSIES0 && (u16)item <= FLOWER_TULIP2) {
        res = TRUE;
    } else if ((u16)item >= DLX_FLOWER_RANGE1_START && (u16)item <= DLX_FLOWER_RANGE1_END) {
        res = TRUE;
    } else if ((u16)item >= DLX_FLOWER_RANGE2_START && (u16)item <= DLX_FLOWER_RANGE2_END) {
        res = TRUE;
    }

    return (u8)res;
}

extern int mNT_ItIsRstStone(mActor_name_t actor) {
    int table_no;

    table_no = mNT_get_itemTableNo(actor);
    return (u8)((u32)(table_no - 0x14) <= 1);
}

/* Deluxe: get food type from food item */
extern int mNT_FoodItem2FoodType(mActor_name_t item) {
    return ((u16)item - ITM_FOOD_START) / 10;
}

/* Deluxe: get food count from food item */
extern int mNT_FoodItem2FoodCount(mActor_name_t item) {
    return ((u16)item - ITM_FOOD_START) % 10 + 1;
}

/* Deluxe: get food item from type index and count */
extern mActor_name_t mNT_FoodCount2FoodItem(int food_idx, int count) {
    return (mActor_name_t)((u16)(food_idx * 10 + count + ITM_FOOD_START - 1));
}

/* Deluxe: try to stack a food item onto a single target slot */
extern int mNT_TryStackFoodSingle(mActor_name_t food_item, mActor_name_t* target) {
    int food_type;
    int food_count;
    int target_type;
    int target_count;

    food_type = mNT_FoodItem2FoodType(food_item);
    food_count = mNT_FoodItem2FoodCount(food_item);

    if ((u16)food_item < ITM_FOOD_START || (u16)food_item > DLX_FOOD_RANGE_END) {
        return FALSE;
    }
    if (food_type < 0 || food_type >= DLX_FOOD_TYPE_NUM) {
        return FALSE;
    }
    if (food_count <= 0 || food_count > DLX_FOOD_COUNT_MAX) {
        return FALSE;
    }
    if (target == NULL) {
        return FALSE;
    }

    if ((u16)*target < ITM_FOOD_START || (u16)*target > DLX_FOOD_RANGE_END) {
        return FALSE;
    }

    target_type = mNT_FoodItem2FoodType(*target);
    target_count = mNT_FoodItem2FoodCount(*target);

    if (target_type != food_type) {
        return FALSE;
    }

    if (food_count + target_count > DLX_FOOD_COUNT_MAX) {
        return FALSE;
    }

    *target = mNT_FoodCount2FoodItem(food_type, food_count + target_count);
    return TRUE;
}

/* Deluxe: try to stack a food item into an array of items */
extern int mNT_TryStackFood(mActor_name_t food_item, mActor_name_t* items, int count) {
    int food_type;
    int food_count;
    int i;
    int target_type;
    int target_count;

    food_type = mNT_FoodItem2FoodType(food_item);
    food_count = mNT_FoodItem2FoodCount(food_item);

    if ((u16)food_item < ITM_FOOD_START || (u16)food_item > DLX_FOOD_RANGE_END) {
        return FALSE;
    }
    if (food_type < 0 || food_type >= DLX_FOOD_TYPE_NUM) {
        return FALSE;
    }
    if (food_count <= 0 || food_count > DLX_FOOD_COUNT_MAX) {
        return FALSE;
    }

    for (i = 0; i < count; i++) {
        target_type = mNT_FoodItem2FoodType(items[i]);
        target_count = mNT_FoodItem2FoodCount(items[i]);

        if (target_type == food_type && food_count + target_count <= DLX_FOOD_COUNT_MAX) {
            items[i] = mNT_FoodCount2FoodItem(food_type, food_count + target_count);
            return TRUE;
        }
    }

    return FALSE;
}

/* Deluxe: check if there's space to stack a food item into an array */
extern int mNT_IsThereSpaceToStackFood(mActor_name_t food_item, mActor_name_t* items, int count) {
    int food_type;
    int food_count;
    int i;
    int target_type;
    int target_count;

    food_type = mNT_FoodItem2FoodType(food_item);
    food_count = mNT_FoodItem2FoodCount(food_item);

    for (i = 0; i < count; i++) {
        target_type = mNT_FoodItem2FoodType(items[i]);
        target_count = mNT_FoodItem2FoodCount(items[i]);

        if (target_type == food_type && food_count + target_count <= DLX_FOOD_COUNT_MAX) {
            return TRUE;
        }
    }

    return FALSE;
}

/* Deluxe: split a food stack, reducing by 1 and returning the single item */
extern int mNT_TrySplitFood(mActor_name_t* food_item_p) {
    int food_type;
    int food_count;
    mActor_name_t item;

    item = *food_item_p;
    if ((u16)item < ITM_FOOD_START || (u16)item > DLX_FOOD_RANGE_END) {
        return 0;
    }

    food_type = mNT_FoodItem2FoodType(item);
    food_count = mNT_FoodItem2FoodCount(item);

    if (food_count <= 1) {
        return 0;
    }

    *food_item_p = mNT_FoodCount2FoodItem(food_type, food_count - 1);
    return mNT_FoodCount2FoodItem(food_type, 1);
}

/* Deluxe: get a new live version of a music disc if conditions are met */
extern mActor_name_t mNT_get_new_music_live_version(int bit_idx) {
    Private_c* priv;
    u32 has_item;

    priv = Common_Get(now_private);
    has_item = mPr_GetPossessionItemSumWithCond(priv, 0, 0);

    if (mLd_PlayerManKindCheck() != 0) {
        return 0;
    }
    if (priv == NULL) {
        return 0;
    }

    /* Check if bit is set in the music available field but not in the presented field */
    if (!((*(u16*)&priv->unused_2412[0] >> bit_idx) & 1)) {
        return 0;
    }
    if ((priv->unused_23CA[0] >> bit_idx) & 1) {
        return 0;
    }
    if ((s32)has_item <= 0) {
        return 0;
    }

    return (mActor_name_t)(ITM_MINIDISK_LIVE_START + bit_idx);
}

/* Deluxe: mark a live music version as presented */
extern void mNT_present_new_music_live_version(int bit_idx) {
    Private_c* priv;
    u16 avail;
    u8 presented;
    u16 mask;

    priv = Common_Get(now_private);
    if (mLd_PlayerManKindCheck() != 0) {
        return;
    }
    if (priv == NULL) {
        return;
    }

    mask = (u16)(1 << bit_idx);
    avail = *(u16*)&priv->unused_2412[0];
    *(u16*)&priv->unused_2412[0] = avail & ~mask;
    presented = priv->unused_23CA[0];
    priv->unused_23CA[0] = presented | (u8)mask;
}

/* Deluxe: get cloth type for an item */
extern int mNT_get_cloth_type(mActor_name_t item, u16 npc_id, int flag) {
    if ((u16)item >= RSV_CLOTH && (u16)item <= RSV_CLOTH7) {
        if (flag != 0) {
            /* Read from save data design by player_no */
            int player_no;
            u8* base;

            player_no = npc_id & 7;
            base = (u8*)&common_data + 0x20000;
            return *(base + player_no * 0x220 + 0x1412);
        } else {
            /* Read from now_private design by NPC player index */
            int design_idx;
            Private_c* priv;

            design_idx = ((u16)npc_id - 0x10E) & 7;
            priv = Common_Get(now_private);
            return *((u8*)priv + design_idx * 0x220 + 0x1252);
        }
    }

    {
        u8* info = mIF_GetInfoForItem(item);
        return (info[6] >> 4) & 0xF;
    }
}
