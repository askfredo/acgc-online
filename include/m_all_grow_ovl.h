#ifndef M_ALL_GROW_OVL_H
#define M_ALL_GROW_OVL_H

#include "types.h"
#include "m_time.h"
#include "m_field_info.h"
#include "m_field_make.h"
#include "m_island.h"
#include "m_olib.h"
#include "m_private.h"

#ifdef __cplusplus
extern "C" {
#endif

#define mAGrw_FOSSIL_NUM 5
#define mAGrw_HANIWA_NUM 3
#define mAGrw_FTR_TREE_NUM 2
#define mAGrw_MAX_PAST_DAYS 5
#define mAGrw_GRASS_TABLE_NUM 12
#define mAGrw_MONEY_TREE_NUM 30
#define mAGrw_MAX_TREES_PER_BLOCK 32
#define mAGrw_ISL_TREE_MAX_LINE 3
#define mAGrw_ISL_MAX_TREES_PER_BLOCK 15
#define mAGrw_DUMP_ITEM_NUM 2
#define mAGrw_GRASS_PER_DAY 5

enum {
  mAGrw_GROW_FIRST,
  mAGrw_GROW,
  mAGrw_GROW_FIRST_FORCE, /* Used when you go back in time */

  mAGrw_GROW_ISLAND_FIRST,
  mAGrw_GROW_ISLAND,
  mAGrw_GROW_ISLAND_FIRST_FORCE, /* Used when you go back in time */

  mAGrw_GROW_NUM
};

enum {
  mAGrw_TREE000,
  mAGrw_TREE001,
  mAGrw_TREE002,
  mAGrw_TREE003,
  mAGrw_TREE004,

  mAGrw_TREE_STAGE_NUM
};

enum {
  mAGrw_AROUND_TOP,
  mAGrw_AROUND_BOTTOM,
  mAGrw_AROUND_LEFT,
  mAGrw_AROUND_RIGHT,

  mAGrw_AROUND_NUM
};

enum {
  mAGrw_FLOWER_TIME,
  mAGrw_NOT_FLOWER_TIME,

  mAGrw_FLOWER_TIME_NUM
};

enum {
  mAGrw_TREE_TYPE_APPLE,
  mAGrw_TREE_TYPE_CHESTNUT, // cherry tree
  mAGrw_TREE_TYPE_PEAR,
  mAGrw_TREE_TYPE_PEACH,
  mAGrw_TREE_TYPE_ORANGE,
  mAGrw_TREE_TYPE_NORMAL,
  mAGrw_TREE_TYPE_MONEY,
  mAGrw_TREE_TYPE_PALM,
  mAGrw_TREE_TYPE_CEDAR,
  mAGrw_TREE_TYPE_GOLD,

  mAGrw_TREE_TYPE_NUM
};

enum {
  mAGrw_KILL_PALM_TREE,
  mAGrw_KILL_CEDAR_TREE,

  mAGrw_KILL_TREE_NUM
};

enum {
  mAGrw_CARP_UNCHANGED,
  mAGrw_CARP_PLACE,
  mAGrw_CARP_REMOVE,

  mAGrw_CARP_NUM
};

enum {
  mAGrw_CHECK_CANCEL_12,
  mAGrw_CHECK_CANCEL_32,
  mAGrw_CHECK_CANCEL_22,
  mAGrw_CHECK_CANCEL_36,
  mAGrw_CHECK_CANCEL_LEFT_45,
  mAGrw_CHECK_CANCEL_RIGHT_45,
  mAGrw_CHECK_CANCEL_46,
  mAGrw_CHECK_CANCEL_23,
  mAGrw_CHECK_CANCEL_57,
  mAGrw_CHECK_CANCEL_68,
  mAGrw_CHECK_CANCEL_77,

  mAGrw_CHECK_CANCEL_NUM
};

typedef struct shine_stone_pos_info {
  u8 block_x;
  u8 block_z;
  u8 ut_x;
  u8 ut_z;
} mAGrw_SSPosInfo_c;

typedef struct allgrow_s {
  mAGrw_SSPosInfo_c shine_pos[TOTAL_PLAYER_NUM];
  mAGrw_SSPosInfo_c stone_pos[TOTAL_PLAYER_NUM];
} mAGrw_AllGrow_c;

typedef struct grow_info_s {
  mFM_fg_c* around_block[mAGrw_AROUND_NUM];

  int check_plant;
  int past_days;
  int block_height;
  int ocean_row;
  int flower_time;
  int spoil_kabu;

  int block_x;
  int block_z;
  int ut_x;
  int ut_z;

  int deposited_item;
  int money_tree_num;
  u8 fossil_record;
  int fossil_num;
  u8 honeycomb_tree_record;
  u8 ftr_tree_record;
  u8 money_stone_spawned;
} mAGrw_GrowInfo_c;

typedef struct carp_block_info_s {
  u8 signboard_in_block:1;
  u8 carp_already_exists:1;
  u8 villager_house_in_block:1;
  u8 signboard_count:5;
} mAGrw_CarpBlockInfo_c;

typedef struct carp_info_s {
  mAGrw_CarpBlockInfo_c block_flags[FG_BLOCK_TOTAL_NUM];
  u8 signboard_blocks;
  u8 carp_blocks;
  u8 villager_house_blocks;
} mAGrw_CarpInfo_c;

/* Deluxe: reset stone timing info */
typedef struct allgrow_reset_stone_s {
  mAGrw_SSPosInfo_c pos_info;
  lbRTC_ymd_c spawn_date;
  lbRTC_ymd_c expire_date;
} mAGrw_ResetStone_c;

/* Deluxe: new grow info for grass/clover/rafflesia system */
typedef struct grow_info_new_s {
  /* 0x00 */ u8 flags;
  /* 0x01 */ u8 monument_bx;
  /* 0x02 */ u8 monument_bz;
  /* 0x03 */ u8 monument_ut_x;
  /* 0x04 */ u8 monument_ut_z;
  /* 0x05 */ u8 rafflesia_bx;
  /* 0x06 */ u8 rafflesia_bz;
  /* 0x07 */ u8 rafflesia_ut_x;
  /* 0x08 */ u8 rafflesia_ut_z;
  /* 0x09 */ u8 pad;
  /* 0x0A */ u16 weed_count;
  /* 0x0C */ u16 tree_count;
  /* 0x0E */ u16 flower_count;
  /* 0x10 */ u16 snowman_count;
  /* 0x12 */ u16 pad2;
  /* 0x14 */ u16* fg_cancel;
  /* 0x18 */ int candidate_count;
} mAGrw_GrowInfoNew_c;

/* Deluxe flower breed types */
enum {
  mAGrw_BREED_TULIP,
  mAGrw_BREED_PANSY,
  mAGrw_BREED_COSMOS,
  mAGrw_BREED_ROSE,
  mAGrw_BREED_LILY,

  mAGrw_BREED_NUM
};

extern void mAGrw_ClearMoneyStoneShineGround();
extern void mAGrw_ClearAllShine_Stone();
extern void mAGrw_RestoreStoneShine(int player_no);
extern void mAGrw_SetHideUtInfo(u16* hide, mActor_name_t* items);
extern void mAGrw_SearchDump(mFI_unit_c* dump_info);
extern void mAGrw_SpoilKabuIsland(Island_c* island);
extern void mAGrw_RenewalFgItem_ovl(lbRTC_time_c* now_time, int* deposit_haniwa);

/* Deluxe functions */
extern int mAGrw_CheckCherryTree(mActor_name_t item, int unused1, int unused2);
extern void mAGrw_KillTree_com(mActor_name_t* item);
extern mActor_name_t mAGrw_RstStone2Stone(mActor_name_t item);
extern void mAGrw_RemoveBlackGoldRoses(void);
extern int mAGrw_BuildMonument(mActor_name_t* item, mAGrw_GrowInfo_c* grow_info);
extern void mAGrw_RemoveMonument(void);
extern int mAGrw_ClassifyFlowerBreed(mActor_name_t item);
extern int mAGrw_Flower2BreedIdx(mActor_name_t item);
extern mActor_name_t mAGrw_FlowerBreedAndIdx2Item(int breed, int idx);
extern void mAGrw_CrossBreedBlackGoldRoses(void);
extern mActor_name_t mAGrw_CrossBreedRegularFlower(mActor_name_t flower_a, mActor_name_t flower_b, int breed);
extern int mAGrw_ChkFlower(mActor_name_t item, int bx, int bz, int ut_x, int ut_z, int breed);
extern int mAGrw_SearchPartnerFlower(int bx, int bz, int ut_x, int ut_z, int breed, int* out_x, int* out_z, mActor_name_t* out_item);
extern int mAGrw_TryBreedFlowers(int pos1, int pos2, int partner_pos1, int partner_pos2, mActor_name_t flower_a, mActor_name_t flower_b, int breed);
extern int mAGrw_CrossBreedFlowersInBlock(int bx, int bz);
extern void mAGrw_CrossBreedFlowers(void);
extern int mAGrw_GetCloverMax(const lbRTC_time_c* now, const lbRTC_time_c* last);
extern void mAGrw_SetCandidateNumInfo(mAGrw_GrowInfoNew_c* info);
extern void mAGrw_GrowInfoGrassAndClovers(const mAGrw_GrowInfoNew_c* info, const lbRTC_time_c* now, const lbRTC_time_c* last);
extern void mAGrw_GrowInfoSpecialFlower(const mAGrw_GrowInfoNew_c* info);
extern void mAGrw_GrowInfoDump(const mAGrw_GrowInfoNew_c* info, lbRTC_time_c* now, lbRTC_time_c* last);
extern void mAGrw_GrowInfoRafflesia(mAGrw_GrowInfoNew_c* info);
extern void mAGrw_ProcessGrowInfo(mAGrw_GrowInfoNew_c* info, lbRTC_time_c* now, lbRTC_time_c* last);
extern int mAGrw_ChoiceEnableSetSpecialFlowerBlock(BlockOrUnit_c* candidates);
extern int mAGrw_ChoiceEnableSetSpecialFlowerUt(BlockOrUnit_c* out_list, const BlockOrUnit_c* block_info, const u16* cancel);
extern void mAGrw_SetSpecialFlower(u16* cancel);
extern void mAGrw_MakeGrassCandidateTable(int* candidate_table, const u16* cancel);
extern void mAGrw_SetRandomGrass(const u16* cancel, int iterations, int remaining);
extern void mAGrw_SetRandomClover(const u16* cancel, int iterations, int remaining);
extern void mAgrw_RenewResetStoneTime(mAGrw_ResetStone_c* rst, lbRTC_ymd_c* date);
extern void mAGrw_InitResetStoneTime(mAGrw_ResetStone_c* rst, lbRTC_ymd_c* date);
extern int mAGrw_CheckResetStoneTime(mAGrw_ResetStone_c* rst);
extern void mAGrw_SetResetStone(mAGrw_ResetStone_c* rst);
extern void mAGrw_MakeGrowInfo(mAGrw_GrowInfoNew_c* info, lbRTC_time_c* now, lbRTC_time_c* last, u16* cancel);

#ifdef __cplusplus
}
#endif

#endif
