#include "m_map_ovl.h"

#include "dolphin/os/OSCache.h"
#include "audio.h"
#include "m_random_field.h"
#include "m_field_info.h"
#include "m_event.h"
#include "m_font.h"
#include "m_player.h"
#include "m_player_lib.h"
#include "game.h"
#include "sys_matrix.h"
#include "m_malloc.h"
#include "m_common_data.h"
#include "_mem.h"

extern u8 kan_tizu_f_TA_tex_txt[];
extern u8 kan_tizu_tst1_TA_tex_txt[];
extern u8 kan_tizu_t_TA_tex_txt[];
extern u8 kan_tizu_tr1_TA_tex_txt[];
extern u8 kan_tizu_fmh_TA_tex_txt[];
extern u8 kan_tizu_c1_TA_tex_txt[];
extern u8 kan_tizu_c2_TA_tex_txt[];
extern u8 kan_tizu_c3_TA_tex_txt[];
extern u8 kan_tizu_c4_TA_tex_txt[];
extern u8 kan_tizu_c5_TA_tex_txt[];
extern u8 kan_tizu_c6_TA_tex_txt[];
extern u8 kan_tizu_c7_TA_tex_txt[];
extern u8 kan_tizu_c1r1_TA_tex_txt[];
extern u8 kan_tizu_c2r1_TA_tex_txt[];
extern u8 kan_tizu_c3r1_TA_tex_txt[];
extern u8 kan_tizu_c4r1_TA_tex_txt[];
extern u8 kan_tizu_c5r1_TA_tex_txt[];
extern u8 kan_tizu_c6r1_TA_tex_txt[];
extern u8 kan_tizu_c7r1_TA_tex_txt[];
extern u8 kan_tizu_c1r2_TA_tex_txt[];
extern u8 kan_tizu_c2r2_TA_tex_txt[];
extern u8 kan_tizu_c3r2_TA_tex_txt[];
extern u8 kan_tizu_c4r2_TA_tex_txt[];
extern u8 kan_tizu_c5r2_TA_tex_txt[];
extern u8 kan_tizu_c1r2_TA_tex_txt[];
extern u8 kan_tizu_c4r2_TA_tex_txt[];
extern u8 kan_tizu_c5r2_TA_tex_txt[];
extern u8 kan_tizu_c6r3_TA_tex_txt[];
extern u8 kan_tizu_c7r3_TA_tex_txt[];
extern u8 kan_tizu_r1_TA_tex_txt[];
extern u8 kan_tizu_r2_TA_tex_txt[];
extern u8 kan_tizu_r2_TA_tex_txt[];
extern u8 kan_tizu_r4_TA_tex_txt[];
extern u8 kan_tizu_r5_TA_tex_txt[];
extern u8 kan_tizu_r6_TA_tex_txt[];
extern u8 kan_tizu_r7_TA_tex_txt[];
extern u8 kan_tizu_r1b_TA_tex_txt[];
extern u8 kan_tizu_r2b_TA_tex_txt[];
extern u8 kan_tizu_r2b_TA_tex_txt[];
extern u8 kan_tizu_r4b_TA_tex_txt[];
extern u8 kan_tizu_r5b_TA_tex_txt[];
extern u8 kan_tizu_r6b_TA_tex_txt[];
extern u8 kan_tizu_r7b_TA_tex_txt[];
extern u8 kan_tizu_c1s_TA_tex_txt[];
extern u8 kan_tizu_c2s_TA_tex_txt[];
extern u8 kan_tizu_c3s_TA_tex_txt[];
extern u8 kan_tizu_c4s_TA_tex_txt[];
extern u8 kan_tizu_c5s_TA_tex_txt[];
extern u8 kan_tizu_c6s_TA_tex_txt[];
extern u8 kan_tizu_c7s_TA_tex_txt[];
extern u8 kan_tizu_m_TA_tex_txt[];
extern u8 kan_tizu_mr1_TA_tex_txt[];
extern u8 kan_tizu_fsh_TA_tex_txt[];
extern u8 kan_tizu_fpk_TA_tex_txt[];
extern u8 kan_tizu_fpo_TA_tex_txt[];
extern u8 kan_tizu_fko_TA_tex_txt[];
extern u8 kan_tizu_pr1_TA_tex_txt[];
extern u8 kan_tizu_pr2_TA_tex_txt[];
extern u8 kan_tizu_pr2_TA_tex_txt[];
extern u8 kan_tizu_pr4_TA_tex_txt[];
extern u8 kan_tizu_pr5_TA_tex_txt[];
extern u8 kan_tizu_pr6_TA_tex_txt[];
extern u8 kan_tizu_pr7_TA_tex_txt[];
extern u8 kan_tizu_mr1b_TA_tex_txt[];
extern u8 kan_tizu_fmu_TA_tex_txt[];
extern u8 kan_tizu_fta_TA_tex_txt[];
extern u8 kan_tizu_tr1b_TA_tex_txt[];
extern u8 kan_tizu_c1r2b_TA_tex_txt[];
extern u8 kan_tizu_c3r1b_TA_tex_txt[];
extern u8 kan_tizu_c4r1b_TA_tex_txt[];
extern u8 kan_tizu_c4r2b_TA_tex_txt[];
extern u8 kan_tizu_c5r2b_TA_tex_txt[];
extern u8 kan_tizu_c6r1b_TA_tex_txt[];
extern u8 kan_tizu_c7r1b_TA_tex_txt[];
extern u8 kan_tizu_mwf_TA_tex_txt[];

static u8* l_map_texture[mFM_BLOCK_TYPE_NUM] = {
    kan_tizu_f_TA_tex_txt, // mFM_BLOCK_TYPE_BORDER_CLIFF_TOP
    kan_tizu_f_TA_tex_txt, // mFM_BLOCK_TYPE_BORDER_CLIFF_RIVER
    kan_tizu_f_TA_tex_txt, // mFM_BLOCK_TYPE_BORDER_CLIFF_LEFT
    kan_tizu_f_TA_tex_txt, // mFM_BLOCK_TYPE_3
    kan_tizu_f_TA_tex_txt, // mFM_BLOCK_TYPE_BORDER_CLIFF_RIGHT
    kan_tizu_f_TA_tex_txt, // mFM_BLOCK_TYPE_BORDER_CLIFF_CORNER_TOP_LEFT
    kan_tizu_f_TA_tex_txt, // mFM_BLOCK_TYPE_6
    kan_tizu_f_TA_tex_txt, // mFM_BLOCK_TYPE_TRACKS
    kan_tizu_f_TA_tex_txt, // mFM_BLOCK_TYPE_BORDER_CLIFF_CORNER_TOP_RIGHT
    kan_tizu_f_TA_tex_txt, // mFM_BLOCK_TYPE_BORDER_CLIFF_LEFT_TUNNEL
    kan_tizu_f_TA_tex_txt, // mFM_BLOCK_TYPE_BORDER_CLIFF_RIGHT_TUNNEL
    kan_tizu_tst1_TA_tex_txt, // mFM_BLOCK_TYPE_TRACKS_STATION
    kan_tizu_t_TA_tex_txt, // mFM_BLOCK_TYPE_TRACKS_DUMP
    kan_tizu_tr1_TA_tex_txt, // mFM_BLOCK_TYPE_TRACKS_RIVER
    kan_tizu_fmh_TA_tex_txt, // mFM_BLOCK_TYPE_PLAYER_HOUSE
    kan_tizu_c1_TA_tex_txt, // mFM_BLOCK_TYPE_CLIFF_HORIZONTAL
    kan_tizu_c2_TA_tex_txt, // mFM_BLOCK_TYPE_CLIFF_BOTTOM_RIGHT_CORNER
    kan_tizu_c3_TA_tex_txt, // mFM_BLOCK_TYPE_CLIFF_VERTICAL_RIGHT
    kan_tizu_c4_TA_tex_txt, // mFM_BLOCK_TYPE_CLIFF_TOP_RIGHT_CORNER
    kan_tizu_c5_TA_tex_txt, // mFM_BLOCK_TYPE_CLIFF_TOP_LEFT_CORNER
    kan_tizu_c6_TA_tex_txt, // mFM_BLOCK_TYPE_CLIFF_VERTICAL_LEFT
    kan_tizu_c7_TA_tex_txt, // mFM_BLOCK_TYPE_CLIFF_BOTTOM_LEFT_CORNER
    kan_tizu_c1r1_TA_tex_txt, // mFM_BLOCK_TYPE_WATERFALL_STRAIGHT_CLIFF_HORIZONTAL
    kan_tizu_c2r1_TA_tex_txt, // mFM_BLOCK_TYPE_WATERFALL_STRAIGHT_CLIFF_BOTTOM_RIGHT_CORNER
    kan_tizu_c3r1_TA_tex_txt, // mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_VERTICAL_RIGHT
    kan_tizu_c4r1_TA_tex_txt, // mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_TOP_RIGHT_CORNER
    kan_tizu_c5r1_TA_tex_txt, // mFM_BLOCK_TYPE_WATERFALL_STRAIGHT_CLIFF_TOP_LEFT_CORNER
    kan_tizu_c6r1_TA_tex_txt, // mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_VERTICAL_LEFT
    kan_tizu_c7r1_TA_tex_txt, // mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_BOTTOM_LEFT_CORNER
    kan_tizu_c1r2_TA_tex_txt, // mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_HORIZONTAL
    kan_tizu_c2r2_TA_tex_txt, // mFM_BLOCK_TYPE_WATERFALL_EAST_CLIFF_BOTTOM_RIGHT_CORNER
    kan_tizu_c3r2_TA_tex_txt, // mFM_BLOCK_TYPE_WATERFALL_EAST_CLIFF_VERTICAL_RIGHT
    kan_tizu_c4r2_TA_tex_txt, // mFM_BLOCK_TYPE_RIVER_EAST_CLIFF_TOP_RIGHT_CORNER
    kan_tizu_c5r2_TA_tex_txt, // mFM_BLOCK_TYPE_RIVER_EAST_CLIFF_TOP_LEFT_CORNER
    kan_tizu_c1r2_TA_tex_txt, // mFM_BLOCK_TYPE_RIVER_WEST_CLIFF_HORIZONTAL
    kan_tizu_c4r2_TA_tex_txt, // mFM_BLOCK_TYPE_RIVER_WEST_CLIFF_TOP_RIGHT_CORNER
    kan_tizu_c5r2_TA_tex_txt, // mFM_BLOCK_TYPE_RIVER_WEST_CLIFF_TOP_LEFT_CORNER
    kan_tizu_c6r3_TA_tex_txt, // mFM_BLOCK_TYPE_WATERFALL_WEST_CLIFF_VERTICAL_LEFT
    kan_tizu_c7r3_TA_tex_txt, // mFM_BLOCK_TYPE_WATERFALL_WEST_CLIFF_BOTTOM_LEFT_CORNER
    kan_tizu_f_TA_tex_txt, // mFM_BLOCK_TYPE_FLAT
    kan_tizu_r1_TA_tex_txt, // mFM_BLOCK_TYPE_RIVER_SOUTH
    kan_tizu_r2_TA_tex_txt, // mFM_BLOCK_TYPE_RIVER_EAST
    kan_tizu_r2_TA_tex_txt, // mFM_BLOCK_TYPE_RIVER_WEST
    kan_tizu_r4_TA_tex_txt, // mFM_BLOCK_TYPE_RIVER_SOUTH_EAST
    kan_tizu_r5_TA_tex_txt, // mFM_BLOCK_TYPE_RIVER_EAST_SOUTH
    kan_tizu_r6_TA_tex_txt, // mFM_BLOCK_TYPE_RIVER_SOUTH_WEST
    kan_tizu_r7_TA_tex_txt, // mFM_BLOCK_TYPE_RIVER_WEST_SOUTH
    kan_tizu_r1b_TA_tex_txt, // mFM_BLOCK_TYPE_RIVER_SOUTH_BRIDGE
    kan_tizu_r2b_TA_tex_txt, // mFM_BLOCK_TYPE_RIVER_EAST_BRIDGE
    kan_tizu_r2b_TA_tex_txt, // mFM_BLOCK_TYPE_RIVER_WEST_BRIDGE
    kan_tizu_r4b_TA_tex_txt, // mFM_BLOCK_TYPE_RIVER_SOUTH_EAST_BRIDGE
    kan_tizu_r5b_TA_tex_txt, // mFM_BLOCK_TYPE_RIVER_EAST_SOUTH_BRIDGE
    kan_tizu_r6b_TA_tex_txt, // mFM_BLOCK_TYPE_RIVER_SOUTH_WEST_BRIDGE
    kan_tizu_r7b_TA_tex_txt, // mFM_BLOCK_TYPE_RIVER_WEST_SOUTH_BRIDGE
    kan_tizu_c1s_TA_tex_txt, // mFM_BLOCK_TYPE_SLOPE_HORIZONTAL
    kan_tizu_c2s_TA_tex_txt, // mFM_BLOCK_TYPE_SLOPE_BOTTOM_RIGHT_CORNER
    kan_tizu_c3s_TA_tex_txt, // mFM_BLOCK_TYPE_SLOPE_VERTICAL_RIGHT
    kan_tizu_c4s_TA_tex_txt, // mFM_BLOCK_TYPE_SLOPE_TOP_RIGHT_CORNER
    kan_tizu_c5s_TA_tex_txt, // mFM_BLOCK_TYPE_SLOPE_TOP_LEFT_CORNER
    kan_tizu_c6s_TA_tex_txt, // mFM_BLOCK_TYPE_SLOPE_VERTICAL_LEFT
    kan_tizu_c7s_TA_tex_txt, // mFM_BLOCK_TYPE_SLOPE_BOTTOM_LEFT_CORNER
    kan_tizu_f_TA_tex_txt, // mFM_BLOCK_TYPE_BORDER_CLIFF_LEFT_TRANSITION
    kan_tizu_f_TA_tex_txt, // mFM_BLOCK_TYPE_BORDER_CLIFF_RIGHT_TRANSITION
    kan_tizu_m_TA_tex_txt, // mFM_BLOCK_TYPE_BEACH
    kan_tizu_mr1_TA_tex_txt, // mFM_BLOCK_TYPE_BEACH_RIVER
    kan_tizu_fsh_TA_tex_txt, // mFM_BLOCK_TYPE_TRACKS_SHOP
    kan_tizu_fpk_TA_tex_txt, // mFM_BLOCK_TYPE_SHRINE
    kan_tizu_fpo_TA_tex_txt, // mFM_BLOCK_TYPE_TRACKS_POST_OFFICE
    kan_tizu_fko_TA_tex_txt, // mFM_BLOCK_TYPE_POLICE_BOX
    kan_tizu_pr1_TA_tex_txt, // mFM_BLOCK_TYPE_POOL_SOUTH
    kan_tizu_pr2_TA_tex_txt, // mFM_BLOCK_TYPE_POOL_EAST
    kan_tizu_pr2_TA_tex_txt, // mFM_BLOCK_TYPE_POOL_WEST
    kan_tizu_pr4_TA_tex_txt, // mFM_BLOCK_TYPE_POOL_SOUTH_EAST
    kan_tizu_pr5_TA_tex_txt, // mFM_BLOCK_TYPE_POOL_EAST_SOUTH
    kan_tizu_pr6_TA_tex_txt, // mFM_BLOCK_TYPE_POOL_SOUTH_WEST
    kan_tizu_pr7_TA_tex_txt, // mFM_BLOCK_TYPE_POOL_WEST_SOUTH
    kan_tizu_f_TA_tex_txt, // mFM_BLOCK_TYPE_TRACKS6
    kan_tizu_r1_TA_tex_txt, // mFM_BLOCK_TYPE_TRACKS7
    kan_tizu_c1_TA_tex_txt, // mFM_BLOCK_TYPE_TRACKS8
    kan_tizu_c1_TA_tex_txt, // mFM_BLOCK_TYPE_TRACKS9
    kan_tizu_c1_TA_tex_txt, // mFM_BLOCK_TYPE_BORDER_CLIFF_OCEAN_LEFT
    kan_tizu_c1_TA_tex_txt, // mFM_BLOCK_TYPE_BORDER_CLIFF_OCEAN_RIGHT
    kan_tizu_mr1b_TA_tex_txt, // mFM_BLOCK_TYPE_BEACH_RIVER_BRIDGE
    kan_tizu_mr1_TA_tex_txt, // mFM_BLOCK_TYPE_OCEAN
    kan_tizu_fmu_TA_tex_txt, // mFM_BLOCK_TYPE_MUSEUM
    kan_tizu_fta_TA_tex_txt, // mFM_BLOCK_TYPE_NEEDLEWORK
    kan_tizu_tr1b_TA_tex_txt, // mFM_BLOCK_TYPE_TRACKS_RIVER_BRIDGE
    kan_tizu_c1r2b_TA_tex_txt, // mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_HORIZONTAL_BRIDGE
    kan_tizu_c3r1b_TA_tex_txt, // mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_VERTICAL_RIGHT_BRIDGE
    kan_tizu_c4r1b_TA_tex_txt, // mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_TOP_RIGHT_CORNER_BRIDGE
    kan_tizu_c4r2b_TA_tex_txt, // mFM_BLOCK_TYPE_RIVER_EAST_CLIFF_TOP_RIGHT_CORNER_BRIDGE
    kan_tizu_c5r2b_TA_tex_txt, // mFM_BLOCK_TYPE_RIVER_EAST_CLIFF_TOP_LEFT_CORNER_BRIDGE
    kan_tizu_c6r1b_TA_tex_txt, // mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_VERTICAL_LEFT_BRIDGE
    kan_tizu_c7r1b_TA_tex_txt, // mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_BOTTOM_LEFT_CORNER_BRIDGE
    kan_tizu_c7r1b_TA_tex_txt, // mFM_BLOCK_TYPE_OCEAN_2
    kan_tizu_c7r1b_TA_tex_txt, // mFM_BLOCK_TYPE_OCEAN_3
    kan_tizu_c7r1b_TA_tex_txt, // mFM_BLOCK_TYPE_OCEAN_4
    kan_tizu_c7r1b_TA_tex_txt, // mFM_BLOCK_TYPE_OCEAN_5
    kan_tizu_c7r1b_TA_tex_txt, // mFM_BLOCK_TYPE_ISLAND_LEFT
    kan_tizu_c7r1b_TA_tex_txt, // mFM_BLOCK_TYPE_ISLAND_RIGHT
    kan_tizu_mwf_TA_tex_txt, // mFM_BLOCK_TYPE_PORT
    kan_tizu_mwf_TA_tex_txt, // mFM_BLOCK_TYPE_SEA_EXCEPTIONAL
    kan_tizu_mwf_TA_tex_txt, // mFM_BLOCK_TYPE_OCEAN_6
    kan_tizu_mwf_TA_tex_txt, // mFM_BLOCK_TYPE_OCEAN_7
    kan_tizu_mwf_TA_tex_txt, // mFM_BLOCK_TYPE_OCEAN_8
    kan_tizu_c1r2b_TA_tex_txt, // mFM_BLOCK_TYPE_RIVER_WEST_CLIFF_HORIZONTAL_BRIDGE
    kan_tizu_c4r2b_TA_tex_txt, // mFM_BLOCK_TYPE_RIVER_WEST_CLIFF_TOP_RIGHT_CORNER_BRIDGE
    kan_tizu_c5r2b_TA_tex_txt, // mFM_BLOCK_TYPE_RIVER_WEST_CLIFF_TOP_LEFT_CORNER_BRIDGE
};

static u8 l_map_pal[mFM_BLOCK_TYPE_NUM] = {
    0, // mFM_BLOCK_TYPE_BORDER_CLIFF_TOP
    0, // mFM_BLOCK_TYPE_BORDER_CLIFF_RIVER
    0, // mFM_BLOCK_TYPE_BORDER_CLIFF_LEFT
    0, // mFM_BLOCK_TYPE_3
    0, // mFM_BLOCK_TYPE_BORDER_CLIFF_RIGHT
    0, // mFM_BLOCK_TYPE_BORDER_CLIFF_CORNER_TOP_LEFT
    0, // mFM_BLOCK_TYPE_6
    0, // mFM_BLOCK_TYPE_TRACKS
    0, // mFM_BLOCK_TYPE_BORDER_CLIFF_CORNER_TOP_RIGHT
    0, // mFM_BLOCK_TYPE_BORDER_CLIFF_LEFT_TUNNEL
    0, // mFM_BLOCK_TYPE_BORDER_CLIFF_RIGHT_TUNNEL
    1, // mFM_BLOCK_TYPE_TRACKS_STATION
    1, // mFM_BLOCK_TYPE_TRACKS_DUMP
    1, // mFM_BLOCK_TYPE_TRACKS_RIVER
    1, // mFM_BLOCK_TYPE_PLAYER_HOUSE
    0, // mFM_BLOCK_TYPE_CLIFF_HORIZONTAL
    0, // mFM_BLOCK_TYPE_CLIFF_BOTTOM_RIGHT_CORNER
    0, // mFM_BLOCK_TYPE_CLIFF_VERTICAL_RIGHT
    0, // mFM_BLOCK_TYPE_CLIFF_TOP_RIGHT_CORNER
    0, // mFM_BLOCK_TYPE_CLIFF_TOP_LEFT_CORNER
    0, // mFM_BLOCK_TYPE_CLIFF_VERTICAL_LEFT
    0, // mFM_BLOCK_TYPE_CLIFF_BOTTOM_LEFT_CORNER
    0, // mFM_BLOCK_TYPE_WATERFALL_STRAIGHT_CLIFF_HORIZONTAL
    0, // mFM_BLOCK_TYPE_WATERFALL_STRAIGHT_CLIFF_BOTTOM_RIGHT_CORNER
    0, // mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_VERTICAL_RIGHT
    0, // mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_TOP_RIGHT_CORNER
    0, // mFM_BLOCK_TYPE_WATERFALL_STRAIGHT_CLIFF_TOP_LEFT_CORNER
    0, // mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_VERTICAL_LEFT
    0, // mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_BOTTOM_LEFT_CORNER
    0, // mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_HORIZONTAL
    0, // mFM_BLOCK_TYPE_WATERFALL_EAST_CLIFF_BOTTOM_RIGHT_CORNER
    0, // mFM_BLOCK_TYPE_WATERFALL_EAST_CLIFF_VERTICAL_RIGHT
    0, // mFM_BLOCK_TYPE_RIVER_EAST_CLIFF_TOP_RIGHT_CORNER
    0, // mFM_BLOCK_TYPE_RIVER_EAST_CLIFF_TOP_LEFT_CORNER
    0, // mFM_BLOCK_TYPE_RIVER_WEST_CLIFF_HORIZONTAL
    0, // mFM_BLOCK_TYPE_RIVER_WEST_CLIFF_TOP_RIGHT_CORNER
    0, // mFM_BLOCK_TYPE_RIVER_WEST_CLIFF_TOP_LEFT_CORNER
    0, // mFM_BLOCK_TYPE_WATERFALL_WEST_CLIFF_VERTICAL_LEFT
    0, // mFM_BLOCK_TYPE_WATERFALL_WEST_CLIFF_BOTTOM_LEFT_CORNER
    0, // mFM_BLOCK_TYPE_FLAT
    0, // mFM_BLOCK_TYPE_RIVER_SOUTH
    0, // mFM_BLOCK_TYPE_RIVER_EAST
    0, // mFM_BLOCK_TYPE_RIVER_WEST
    0, // mFM_BLOCK_TYPE_RIVER_SOUTH_EAST
    0, // mFM_BLOCK_TYPE_RIVER_EAST_SOUTH
    0, // mFM_BLOCK_TYPE_RIVER_SOUTH_WEST
    0, // mFM_BLOCK_TYPE_RIVER_WEST_SOUTH
    0, // mFM_BLOCK_TYPE_RIVER_SOUTH_BRIDGE
    0, // mFM_BLOCK_TYPE_RIVER_EAST_BRIDGE
    0, // mFM_BLOCK_TYPE_RIVER_WEST_BRIDGE
    0, // mFM_BLOCK_TYPE_RIVER_SOUTH_EAST_BRIDGE
    0, // mFM_BLOCK_TYPE_RIVER_EAST_SOUTH_BRIDGE
    0, // mFM_BLOCK_TYPE_RIVER_SOUTH_WEST_BRIDGE
    0, // mFM_BLOCK_TYPE_RIVER_WEST_SOUTH_BRIDGE
    0, // mFM_BLOCK_TYPE_SLOPE_HORIZONTAL
    0, // mFM_BLOCK_TYPE_SLOPE_BOTTOM_RIGHT_CORNER
    0, // mFM_BLOCK_TYPE_SLOPE_VERTICAL_RIGHT
    0, // mFM_BLOCK_TYPE_SLOPE_TOP_RIGHT_CORNER
    0, // mFM_BLOCK_TYPE_SLOPE_TOP_LEFT_CORNER
    0, // mFM_BLOCK_TYPE_SLOPE_VERTICAL_LEFT
    0, // mFM_BLOCK_TYPE_SLOPE_BOTTOM_LEFT_CORNER
    0, // mFM_BLOCK_TYPE_BORDER_CLIFF_LEFT_TRANSITION
    0, // mFM_BLOCK_TYPE_BORDER_CLIFF_RIGHT_TRANSITION
    1, // mFM_BLOCK_TYPE_BEACH
    1, // mFM_BLOCK_TYPE_BEACH_RIVER
    1, // mFM_BLOCK_TYPE_TRACKS_SHOP
    1, // mFM_BLOCK_TYPE_SHRINE
    1, // mFM_BLOCK_TYPE_TRACKS_POST_OFFICE
    1, // mFM_BLOCK_TYPE_POLICE_BOX
    0, // mFM_BLOCK_TYPE_POOL_SOUTH
    0, // mFM_BLOCK_TYPE_POOL_EAST
    0, // mFM_BLOCK_TYPE_POOL_WEST
    0, // mFM_BLOCK_TYPE_POOL_SOUTH_EAST
    0, // mFM_BLOCK_TYPE_POOL_EAST_SOUTH
    0, // mFM_BLOCK_TYPE_POOL_SOUTH_WEST
    0, // mFM_BLOCK_TYPE_POOL_WEST_SOUTH
    0, // mFM_BLOCK_TYPE_TRACKS6
    0, // mFM_BLOCK_TYPE_TRACKS7
    0, // mFM_BLOCK_TYPE_TRACKS8
    0, // mFM_BLOCK_TYPE_TRACKS9
    0, // mFM_BLOCK_TYPE_BORDER_CLIFF_OCEAN_LEFT
    0, // mFM_BLOCK_TYPE_BORDER_CLIFF_OCEAN_RIGHT
    1, // mFM_BLOCK_TYPE_BEACH_RIVER_BRIDGE
    1, // mFM_BLOCK_TYPE_OCEAN
    1, // mFM_BLOCK_TYPE_MUSEUM
    1, // mFM_BLOCK_TYPE_NEEDLEWORK
    1, // mFM_BLOCK_TYPE_TRACKS_RIVER_BRIDGE
    0, // mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_HORIZONTAL_BRIDGE
    0, // mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_VERTICAL_RIGHT_BRIDGE
    0, // mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_TOP_RIGHT_CORNER_BRIDGE
    0, // mFM_BLOCK_TYPE_RIVER_EAST_CLIFF_TOP_RIGHT_CORNER_BRIDGE
    0, // mFM_BLOCK_TYPE_RIVER_EAST_CLIFF_TOP_LEFT_CORNER_BRIDGE
    0, // mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_VERTICAL_LEFT_BRIDGE
    0, // mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_BOTTOM_LEFT_CORNER_BRIDGE
    0, // mFM_BLOCK_TYPE_OCEAN_2
    0, // mFM_BLOCK_TYPE_OCEAN_3
    0, // mFM_BLOCK_TYPE_OCEAN_4
    0, // mFM_BLOCK_TYPE_OCEAN_5
    0, // mFM_BLOCK_TYPE_ISLAND_LEFT
    0, // mFM_BLOCK_TYPE_ISLAND_RIGHT
    1, // mFM_BLOCK_TYPE_PORT
    1, // mFM_BLOCK_TYPE_SEA_EXCEPTIONAL
    1, // mFM_BLOCK_TYPE_OCEAN_6
    1, // mFM_BLOCK_TYPE_OCEAN_7
    1, // mFM_BLOCK_TYPE_OCEAN_8
    0, // mFM_BLOCK_TYPE_RIVER_WEST_CLIFF_HORIZONTAL_BRIDGE
    0, // mFM_BLOCK_TYPE_RIVER_WEST_CLIFF_TOP_RIGHT_CORNER_BRIDGE
    0, // mFM_BLOCK_TYPE_RIVER_WEST_CLIFF_TOP_LEFT_CORNER_BRIDGE
};

static u8 pluss_bridge[mFM_BLOCK_TYPE_NUM] = {
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_BORDER_CLIFF_TOP
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_BORDER_CLIFF_RIVER
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_BORDER_CLIFF_LEFT
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_3
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_BORDER_CLIFF_RIGHT
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_BORDER_CLIFF_CORNER_TOP_LEFT
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_6
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_TRACKS
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_BORDER_CLIFF_CORNER_TOP_RIGHT
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_BORDER_CLIFF_LEFT_TUNNEL
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_BORDER_CLIFF_RIGHT_TUNNEL
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_TRACKS_STATION
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_TRACKS_DUMP
    mFM_BLOCK_TYPE_TRACKS_RIVER_BRIDGE, // mFM_BLOCK_TYPE_TRACKS_RIVER
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_PLAYER_HOUSE
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_CLIFF_HORIZONTAL
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_CLIFF_BOTTOM_RIGHT_CORNER
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_CLIFF_VERTICAL_RIGHT
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_CLIFF_TOP_RIGHT_CORNER
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_CLIFF_TOP_LEFT_CORNER
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_CLIFF_VERTICAL_LEFT
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_CLIFF_BOTTOM_LEFT_CORNER
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_WATERFALL_STRAIGHT_CLIFF_HORIZONTAL
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_WATERFALL_STRAIGHT_CLIFF_BOTTOM_RIGHT_CORNER
    mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_VERTICAL_RIGHT_BRIDGE, // mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_VERTICAL_RIGHT
    mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_TOP_RIGHT_CORNER_BRIDGE, // mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_TOP_RIGHT_CORNER
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_WATERFALL_STRAIGHT_CLIFF_TOP_LEFT_CORNER
    mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_VERTICAL_LEFT_BRIDGE, // mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_VERTICAL_LEFT
    mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_BOTTOM_LEFT_CORNER_BRIDGE, // mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_BOTTOM_LEFT_CORNER
    mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_HORIZONTAL_BRIDGE, // mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_HORIZONTAL
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_WATERFALL_EAST_CLIFF_BOTTOM_RIGHT_CORNER
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_WATERFALL_EAST_CLIFF_VERTICAL_RIGHT
    mFM_BLOCK_TYPE_RIVER_EAST_CLIFF_TOP_RIGHT_CORNER_BRIDGE, // mFM_BLOCK_TYPE_RIVER_EAST_CLIFF_TOP_RIGHT_CORNER
    mFM_BLOCK_TYPE_RIVER_EAST_CLIFF_TOP_LEFT_CORNER_BRIDGE, // mFM_BLOCK_TYPE_RIVER_EAST_CLIFF_TOP_LEFT_CORNER
    mFM_BLOCK_TYPE_RIVER_WEST_CLIFF_HORIZONTAL_BRIDGE, // mFM_BLOCK_TYPE_RIVER_WEST_CLIFF_HORIZONTAL
    mFM_BLOCK_TYPE_RIVER_WEST_CLIFF_TOP_RIGHT_CORNER_BRIDGE, // mFM_BLOCK_TYPE_RIVER_WEST_CLIFF_TOP_RIGHT_CORNER
    mFM_BLOCK_TYPE_RIVER_WEST_CLIFF_TOP_LEFT_CORNER_BRIDGE, // mFM_BLOCK_TYPE_RIVER_WEST_CLIFF_TOP_LEFT_CORNER
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_WATERFALL_WEST_CLIFF_VERTICAL_LEFT
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_WATERFALL_WEST_CLIFF_BOTTOM_LEFT_CORNER
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_FLAT
    mFM_BLOCK_TYPE_RIVER_SOUTH_BRIDGE, // mFM_BLOCK_TYPE_RIVER_SOUTH
    mFM_BLOCK_TYPE_RIVER_EAST_BRIDGE, // mFM_BLOCK_TYPE_RIVER_EAST
    mFM_BLOCK_TYPE_RIVER_WEST_BRIDGE, // mFM_BLOCK_TYPE_RIVER_WEST
    mFM_BLOCK_TYPE_RIVER_SOUTH_EAST_BRIDGE, // mFM_BLOCK_TYPE_RIVER_SOUTH_EAST
    mFM_BLOCK_TYPE_RIVER_EAST_SOUTH_BRIDGE, // mFM_BLOCK_TYPE_RIVER_EAST_SOUTH
    mFM_BLOCK_TYPE_RIVER_SOUTH_WEST_BRIDGE, // mFM_BLOCK_TYPE_RIVER_SOUTH_WEST
    mFM_BLOCK_TYPE_RIVER_WEST_SOUTH_BRIDGE, // mFM_BLOCK_TYPE_RIVER_WEST_SOUTH
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_RIVER_SOUTH_BRIDGE
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_RIVER_EAST_BRIDGE
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_RIVER_WEST_BRIDGE
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_RIVER_SOUTH_EAST_BRIDGE
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_RIVER_EAST_SOUTH_BRIDGE
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_RIVER_SOUTH_WEST_BRIDGE
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_RIVER_WEST_SOUTH_BRIDGE
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_SLOPE_HORIZONTAL
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_SLOPE_BOTTOM_RIGHT_CORNER
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_SLOPE_VERTICAL_RIGHT
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_SLOPE_TOP_RIGHT_CORNER
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_SLOPE_TOP_LEFT_CORNER
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_SLOPE_VERTICAL_LEFT
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_SLOPE_BOTTOM_LEFT_CORNER
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_BORDER_CLIFF_LEFT_TRANSITION
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_BORDER_CLIFF_RIGHT_TRANSITION
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_BEACH
    mFM_BLOCK_TYPE_BEACH_RIVER_BRIDGE, // mFM_BLOCK_TYPE_BEACH_RIVER
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_TRACKS_SHOP
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_SHRINE
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_TRACKS_POST_OFFICE
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_POLICE_BOX
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_POOL_SOUTH
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_POOL_EAST
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_POOL_WEST
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_POOL_SOUTH_EAST
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_POOL_EAST_SOUTH
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_POOL_SOUTH_WEST
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_POOL_WEST_SOUTH
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_TRACKS6
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_TRACKS7
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_TRACKS8
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_TRACKS9
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_BORDER_CLIFF_OCEAN_LEFT
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_BORDER_CLIFF_OCEAN_RIGHT
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_BEACH_RIVER_BRIDGE
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_OCEAN
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_MUSEUM
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_NEEDLEWORK
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_TRACKS_RIVER_BRIDGE
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_HORIZONTAL_BRIDGE
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_VERTICAL_RIGHT_BRIDGE
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_TOP_RIGHT_CORNER_BRIDGE
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_RIVER_EAST_CLIFF_TOP_RIGHT_CORNER_BRIDGE
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_RIVER_EAST_CLIFF_TOP_LEFT_CORNER_BRIDGE
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_VERTICAL_LEFT_BRIDGE
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_BOTTOM_LEFT_CORNER_BRIDGE
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_OCEAN_2
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_OCEAN_3
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_OCEAN_4
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_OCEAN_5
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_ISLAND_LEFT
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_ISLAND_RIGHT
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_PORT
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_SEA_EXCEPTIONAL
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_OCEAN_6
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_OCEAN_7
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_OCEAN_8
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_RIVER_WEST_CLIFF_HORIZONTAL_BRIDGE
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_RIVER_WEST_CLIFF_TOP_RIGHT_CORNER_BRIDGE
    mFM_BLOCK_TYPE_NONE, // mFM_BLOCK_TYPE_RIVER_WEST_CLIFF_TOP_LEFT_CORNER_BRIDGE
};

extern Gfx kan_win_model[];
extern Gfx kan_win_model2[];

static Gfx* kan_win_wakuT_model_p;
static Gfx* kan_win_waku1T_model_p;
static Gfx* kan_win_waku2T_model_p;
static Gfx* kan_win_waku3T_model_p;
static Gfx* kan_win_waku4T_model_p;
static Gfx* kan_win_color0_mode_p;
static Gfx* kan_win_color1_mode_p;

extern Gfx kan_win_wakuT_model[];
extern Gfx kan_waku_w1T_model[];
extern Gfx kan_waku_w2T_model[];
extern Gfx kan_waku_w3T_model[];
extern Gfx kan_waku_w4T_model[];
extern Gfx kan_win_color0_mode[];
extern Gfx kan_win_color1_mode[];

extern mMP_HousePos_c mMP_house_pos_list[];

extern Gfx kan_win_mode[];

static u16 kan_tizu1_pal[16] ATTRIBUTE_ALIGN(32) = {
    0x0000, 0xc3b0, 0x9e87, 0xd294, 0xca52, 0xbdef, 0xb5ad, 0xb9ce,
    0xa2e8, 0xb18c, 0xa705, 0xdaff, 0xab4a, 0xffff, 0xb635, 0xbe9f,
};

static u16 kan_tizu2_pal[16] ATTRIBUTE_ALIGN(32) = {
    0x0000, 0xc3b0, 0x9e87, 0xc94a, 0xe70d, 0xe1ce, 0xb5ad, 0xb195,
    0xf390, 0xb18c, 0xa705, 0xffff, 0xd54a, 0xdaff, 0xb635, 0xbe9f,
};

static u16* l_kan_tizu_pal[2] = { kan_tizu1_pal, kan_tizu2_pal };

extern u8 kan_win_suuji1_tex_rgb_ia8[];
extern u8 kan_win_suuji2_tex_rgb_ia8[];
extern u8 kan_win_suuji3_tex_rgb_ia8[];
extern u8 kan_win_suuji4_tex_rgb_ia8[];
extern u8 kan_win_suuji5_tex_rgb_ia8[];

static u8* kan_win_evw_anime_1_tex_table[FG_BLOCK_X_NUM] = {
    kan_win_suuji1_tex_rgb_ia8, kan_win_suuji2_tex_rgb_ia8, kan_win_suuji3_tex_rgb_ia8,
    kan_win_suuji4_tex_rgb_ia8, kan_win_suuji5_tex_rgb_ia8,
};

extern u8 kan_win_a_tex_rgb_ia8[];
extern u8 kan_win_b_tex_rgb_ia8[];
extern u8 kan_win_c_tex_rgb_ia8[];
extern u8 kan_win_d_tex_rgb_ia8[];
extern u8 kan_win_e_tex_rgb_ia8[];
extern u8 kan_win_f_tex_rgb_ia8[];

static u8* kan_win_evw_anime_2_tex_table[FG_BLOCK_Z_NUM] = {
    kan_win_a_tex_rgb_ia8, kan_win_b_tex_rgb_ia8, kan_win_c_tex_rgb_ia8,
    kan_win_d_tex_rgb_ia8, kan_win_e_tex_rgb_ia8, kan_win_f_tex_rgb_ia8,
};

extern Gfx kan_win_npcT_1_model[];
extern Gfx kan_win_npcT_2_model[];
extern Gfx kan_win_npcT_3_model[];

extern Gfx kan_win_npc2T_1_model[];
extern Gfx kan_win_npc2T_2_model[];
extern Gfx kan_win_npc2T_3_model[];

static Gfx* kan_win_npcT_table[] = { kan_win_npcT_1_model, kan_win_npcT_2_model, kan_win_npcT_3_model };

static Gfx* kan_win_npc2T_table[] = { kan_win_npc2T_1_model, kan_win_npc2T_2_model, kan_win_npc2T_3_model };

extern Gfx kan_win_genzaiT_model[];

extern Gfx kan_tizu_mode[];
extern Gfx kan_tizu_model[];

extern Gfx kan_win_cursorT_model[];

extern Gfx kan_win_playerT_model[];
extern Gfx kan_win_omiseT_model[];
extern Gfx kan_win_koubanT_model[];
extern Gfx kan_win_yuuT_model[];
extern Gfx kan_win_yashiroT_model[];
extern Gfx kan_win_ekiT_model[];
extern Gfx kan_win_gomiT_model[];
extern Gfx kan_win_mu_model[];
extern Gfx kan_win_ta_model[];
extern Gfx kan_win_funeT_model[];

static mMP_LabelWord_c mMP_label_word_npc = { -92.0f, -19.0f, NULL, 0 };

static mMP_Label_c mMP_label_npc = { -98.0f, -24.0f, kan_win_npcT_1_model, { &mMP_label_word_npc, NULL } };

static mMP_LabelWord_c mMP_label_word_player = { -92.0f, -21.5f, NULL, 0 };

static mMP_Label_c mMP_label_player = { -98.0f, -26.5f, kan_win_playerT_model, { &mMP_label_word_player, NULL } };

static u8 mMP_label_str_shop[4] = { CHAR_S, CHAR_h, CHAR_o, CHAR_p };

static mMP_LabelWord_c mMP_label_word_shop = { -83.0f, -25.0f, mMP_label_str_shop, 4 };

static mMP_Label_c mMP_label_shop = { -93.0f, -30.0f, kan_win_omiseT_model, { &mMP_label_word_shop, NULL } };

static u8 mMP_label_str_police[6] = { CHAR_P, CHAR_o, CHAR_l, CHAR_i, CHAR_c, CHAR_e };

static mMP_LabelWord_c mMP_label_word_police = { -83.0f, -19.0f, mMP_label_str_police, 6 };

static u8 mMP_label_str_police2[7] = { CHAR_S, CHAR_t, CHAR_a, CHAR_t, CHAR_i, CHAR_o, CHAR_n };

static mMP_LabelWord_c mMP_label_word_police2 = { -83.0f, -31.0f, mMP_label_str_police2, 7 };

static mMP_Label_c mMP_label_police = {
    -93.0f, -30.0f, kan_win_koubanT_model, { &mMP_label_word_police, &mMP_label_word_police2 }
};

static u8 mMP_label_str_post[4] = { CHAR_P, CHAR_o, CHAR_s, CHAR_t };

static mMP_LabelWord_c mMP_label_word_post = { -83.0f, -19.0f, mMP_label_str_post, 4 };

static u8 mMP_label_str_post2[6] = { CHAR_O, CHAR_f, CHAR_f, CHAR_i, CHAR_c, CHAR_e };

static mMP_LabelWord_c mMP_label_word_post2 = { -83.0f, -31.0f, mMP_label_str_post2, 6 };

static mMP_Label_c mMP_label_post = {
    -93.0f, -30.0f, kan_win_yuuT_model, { &mMP_label_word_post, &mMP_label_word_post2 }
};

static u8 mMP_label_str_shrine[7] = { CHAR_W, CHAR_i, CHAR_s, CHAR_h, CHAR_i, CHAR_n, CHAR_g };

static mMP_LabelWord_c mMP_label_word_shrine = { -83.0f, -19.0f, mMP_label_str_shrine, 7 };

static u8 mMP_label_str_shrine2[4] = { CHAR_W, CHAR_e, CHAR_l, CHAR_l };

static mMP_LabelWord_c mMP_label_word_shrine2 = { -83.0f, -31.0f, mMP_label_str_shrine2, 4 };

static mMP_Label_c mMP_label_shrine = {
    -93.0f, -30.0f, kan_win_yashiroT_model, { &mMP_label_word_shrine, &mMP_label_word_shrine2 }
};

static u8 mMP_label_str_station[5] = { CHAR_T, CHAR_r, CHAR_a, CHAR_i, CHAR_n };

static mMP_LabelWord_c mMP_label_word_station = { -83.0f, -19.0f, mMP_label_str_station, 5 };

static u8 mMP_label_str_station2[7] = { CHAR_S, CHAR_t, CHAR_a, CHAR_t, CHAR_i, CHAR_o, CHAR_n };

static mMP_LabelWord_c mMP_label_word_station2 = { -83.0f, -31.0f, mMP_label_str_station2, 7 };

static mMP_Label_c mMP_label_station = {
    -93.0f, -31.0f, kan_win_ekiT_model, { &mMP_label_word_station, &mMP_label_word_station2 }
};

static u8 mMP_label_str_junk[4] = { CHAR_D, CHAR_u, CHAR_m, CHAR_p };

static mMP_LabelWord_c mMP_label_word_junk = { -83.0f, -25.0f, mMP_label_str_junk, 4 };

static mMP_Label_c mMP_label_junk = { -93.0f, -30.0f, kan_win_gomiT_model, { &mMP_label_word_junk, NULL } };

static u8 mMP_label_str_museum[6] = { CHAR_M, CHAR_u, CHAR_s, CHAR_e, CHAR_u, CHAR_m };

static mMP_LabelWord_c mMP_label_word_museum = { -83.0f, -25.0f, mMP_label_str_museum, 6 };

static mMP_Label_c mMP_label_museum = { -93.0f, -30.0f, kan_win_mu_model, { &mMP_label_word_museum, NULL } };

static u8 mMP_label_str_needle[6] = { CHAR_T, CHAR_a, CHAR_i, CHAR_l, CHAR_o, CHAR_r };

static mMP_LabelWord_c mMP_label_word_needle = { -83.0f, -25.0f, mMP_label_str_needle, 6 };

static mMP_Label_c mMP_label_needle = { -93.0f, -30.0f, kan_win_ta_model, { &mMP_label_word_needle, NULL } };

static u8 mMP_label_str_port[4] = { CHAR_D, CHAR_o, CHAR_c, CHAR_k };

static mMP_LabelWord_c mMP_label_word_port = { -83.0f, -25.0f, mMP_label_str_port, 4 };

static mMP_Label_c mMP_label_port = { -93.0f, -30.0f, kan_win_funeT_model, { &mMP_label_word_port, NULL } };

static mMP_Label_c* mMP_label_data[mMP_LABEL_NUM] = {
    &mMP_label_npc,     &mMP_label_player, &mMP_label_shop,   &mMP_label_police, &mMP_label_post, &mMP_label_shrine,
    &mMP_label_station, &mMP_label_junk,   &mMP_label_museum, &mMP_label_needle, &mMP_label_port,
};

#if VERSION >= VER_DELUXE
typedef struct structure_info_entry_s {
    u16 fg_min;
    u16 fg_max;
    u16 item_min;
    u16 item_max;
    Gfx* gfx;
    f32 ofs_x;
    f32 ofs_z;
} mMP_StructureInfoEntry_c;
#endif

/* Structure overlay icons — model data not yet created, disabled for now */
#if 0
#define STRUCTURE_INFO_NUM 9

typedef struct structure_pos_entry_s {
    u16 item;
    u8 ut_x;
    u8 ut_z;
} mMP_StructurePosEntry_c;

#define STRUCTURE_POS_MAX 15

extern Gfx kan_win_house_model[];
extern Gfx kan_win_house2_model[];
extern Gfx kan_win_shrine_model[];
extern Gfx kan_win_museum2_model[];
extern Gfx kan_win_police_model[];
extern Gfx kan_win_dock_model[];
extern Gfx kan_win_postoffice_model[];
extern Gfx kan_win_tailor_model[];
extern Gfx kan_win_shop_model[];

static const mMP_StructureInfoEntry_c structure_info[STRUCTURE_INFO_NUM] = {
    { 0x5800, 0x5803, 0xF0F3, 0xF0F6, kan_win_house_model, -1.0f, 0.0f },
    { 0x5804, 0x5807, 0xF0F7, 0xF0FA, kan_win_house2_model, -2.0f, -4.0f },
    { 0x5809, 0x5809, 0xF100, 0xF100, kan_win_shrine_model, 0.0f, -2.0f },
    { 0x5825, 0x5825, 0xF103, 0xF103, kan_win_museum2_model, 0.0f, -4.0f },
    { 0x5808, 0x5808, 0xF0FF, 0xF0FF, kan_win_police_model, -1.0f, -2.0f },
    { 0x583B, 0x583B, 0xF115, 0xF115, kan_win_dock_model, -4.5f, -6.0f },
    { 0x580C, 0x580C, 0xF101, 0xF101, kan_win_postoffice_model, 0.0f, 0.0f },
    { 0x584A, 0x584A, 0xF120, 0xF120, kan_win_tailor_model, 0.0f, -4.0f },
    { 0x584D, 0x584D, 0xF0FF, 0xF0FF, kan_win_shop_model, 0.0f, -4.0f },
};
#endif

#if VERSION >= VER_DELUXE
/* CI4 32x32 texture = 512 bytes per block */
#define MAP_PREVIEW_TEX_SIZE 512
static u8 block_preview_textures[FG_BLOCK_TOTAL_NUM * MAP_PREVIEW_TEX_SIZE] ATTRIBUTE_ALIGN(32);
#endif

#if VERSION >= VER_DELUXE

/* Preview palette indices for type-based rendering */
enum {
    PREV_PAL_TRANSPARENT = 0,
    PREV_PAL_GRASS       = 1,
    PREV_PAL_DARK_GREEN  = 2,
    PREV_PAL_MED_GREEN   = 3,
    PREV_PAL_RIVER       = 4,
    PREV_PAL_OCEAN       = 5,
    PREV_PAL_CLIFF       = 6,
    PREV_PAL_SAND        = 7,
    PREV_PAL_TRACKS      = 8,
    PREV_PAL_BUILDING    = 9,
    PREV_PAL_SLOPE       = 10,
    PREV_PAL_BRIDGE      = 11,
    PREV_PAL_WATERFALL   = 12,
    PREV_PAL_WHITE       = 13,
    PREV_PAL_POOL        = 14,
    PREV_PAL_LIGHT_GREEN = 15,
};

/* RGB5A3 colors for preview palette */
static u16 preview_pal_colors[16] ATTRIBUTE_ALIGN(32) = {
    0x0000, /* 0:  transparent */
    0xC3B0, /* 1:  bright green (grass) */
    0x9E87, /* 2:  dark green */
    0xA2E8, /* 3:  medium green */
    0x953C, /* 4:  blue (river) */
    0x88B6, /* 5:  dark blue (ocean) */
    0xC587, /* 6:  brown (cliff) */
    0xEEF0, /* 7:  tan (sand/beach) */
    0xB5AD, /* 8:  grey (tracks) */
    0xE6D2, /* 9:  red-brown (building) */
    0xD20A, /* 10: light brown (slope) */
    0xBD45, /* 11: dark brown (bridge) */
    0xAD7C, /* 12: cyan (waterfall) */
    0xFFFF, /* 13: white */
    0x9D5C, /* 14: teal (pool) */
    0xB3B0, /* 15: light green */
};

/* Fill a 32x32 CI4 texture (512 bytes) with GX 8x8 tile swizzling.
   stripe_dir: 0=none, 1=vertical, 2=horizontal, 3=corner SE, 4=corner SW */
static void mMP_fillPreviewBlock(u8* dest, u8 base_color,
                                 u8 stripe_color, int stripe_dir) {
    int x;
    int y;
    int tile_x;
    int tile_y;
    int in_x;
    int in_y;
    int tile_idx;
    int byte_ofs;
    u8 color;
    int is_stripe;
    int i;

    for (i = 0; i < MAP_PREVIEW_TEX_SIZE; i++) {
        dest[i] = 0;
    }

    for (y = 0; y < 32; y++) {
        for (x = 0; x < 32; x++) {
            color = base_color;
            is_stripe = FALSE;

            switch (stripe_dir) {
                case 1: /* vertical (south-flowing river) */
                    if (x >= 12 && x <= 19) {
                        is_stripe = TRUE;
                    }
                    break;
                case 2: /* horizontal (east/west river) */
                    if (y >= 12 && y <= 19) {
                        is_stripe = TRUE;
                    }
                    break;
                case 3: /* corner: vertical top + horizontal right */
                    if ((x >= 12 && x <= 19 && y <= 19) ||
                        (y >= 12 && y <= 19 && x >= 12)) {
                        is_stripe = TRUE;
                    }
                    break;
                case 4: /* corner: vertical top + horizontal left */
                    if ((x >= 12 && x <= 19 && y <= 19) ||
                        (y >= 12 && y <= 19 && x <= 19)) {
                        is_stripe = TRUE;
                    }
                    break;
            }

            if (is_stripe) {
                color = stripe_color;
            }

            /* CI4 GX tile swizzle: 8x8 tiles, 4 bits per pixel */
            tile_x = x >> 3;
            tile_y = y >> 3;
            in_x = x & 7;
            in_y = y & 7;
            tile_idx = tile_y * 4 + tile_x;
            byte_ofs = tile_idx * 32 + in_y * 4 + (in_x >> 1);

            if (in_x & 1) {
                dest[byte_ofs] |= (color & 0x0F);
            } else {
                dest[byte_ofs] |= ((color & 0x0F) << 4);
            }
        }
    }
}

static void mMP_prepareBlockPreviewFromType(u8 block_type, u8* dest) {
    u8 base;
    u8 stripe;
    int dir;

    base = PREV_PAL_GRASS;
    stripe = PREV_PAL_RIVER;
    dir = 0;

    switch (block_type) {
        /* Flat grass */
        case mFM_BLOCK_TYPE_FLAT:
        case mFM_BLOCK_TYPE_PLAYER_HOUSE:
            base = PREV_PAL_GRASS;
            break;

        /* Rivers - vertical (south-flowing) */
        case mFM_BLOCK_TYPE_RIVER_SOUTH:
            dir = 1;
            break;
        case mFM_BLOCK_TYPE_RIVER_SOUTH_BRIDGE:
            dir = 1;
            stripe = PREV_PAL_BRIDGE;
            break;

        /* Rivers - horizontal */
        case mFM_BLOCK_TYPE_RIVER_EAST:
        case mFM_BLOCK_TYPE_RIVER_WEST:
            dir = 2;
            break;
        case mFM_BLOCK_TYPE_RIVER_EAST_BRIDGE:
        case mFM_BLOCK_TYPE_RIVER_WEST_BRIDGE:
            dir = 2;
            stripe = PREV_PAL_BRIDGE;
            break;

        /* River corners */
        case mFM_BLOCK_TYPE_RIVER_SOUTH_EAST:
        case mFM_BLOCK_TYPE_RIVER_EAST_SOUTH:
        case mFM_BLOCK_TYPE_RIVER_SOUTH_EAST_BRIDGE:
        case mFM_BLOCK_TYPE_RIVER_EAST_SOUTH_BRIDGE:
            dir = 3;
            break;
        case mFM_BLOCK_TYPE_RIVER_SOUTH_WEST:
        case mFM_BLOCK_TYPE_RIVER_WEST_SOUTH:
        case mFM_BLOCK_TYPE_RIVER_SOUTH_WEST_BRIDGE:
        case mFM_BLOCK_TYPE_RIVER_WEST_SOUTH_BRIDGE:
            dir = 4;
            break;

        /* Cliff-river combos */
        case mFM_BLOCK_TYPE_WATERFALL_STRAIGHT_CLIFF_HORIZONTAL:
        case mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_HORIZONTAL:
        case mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_HORIZONTAL_BRIDGE:
        case mFM_BLOCK_TYPE_RIVER_WEST_CLIFF_HORIZONTAL:
        case mFM_BLOCK_TYPE_RIVER_WEST_CLIFF_HORIZONTAL_BRIDGE:
            base = PREV_PAL_CLIFF;
            dir = 2;
            break;
        case mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_VERTICAL_RIGHT:
        case mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_VERTICAL_RIGHT_BRIDGE:
        case mFM_BLOCK_TYPE_WATERFALL_EAST_CLIFF_VERTICAL_RIGHT:
        case mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_VERTICAL_LEFT:
        case mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_VERTICAL_LEFT_BRIDGE:
        case mFM_BLOCK_TYPE_WATERFALL_WEST_CLIFF_VERTICAL_LEFT:
            base = PREV_PAL_CLIFF;
            dir = 1;
            break;

        /* Cliff-river corners */
        case mFM_BLOCK_TYPE_WATERFALL_STRAIGHT_CLIFF_BOTTOM_RIGHT_CORNER:
        case mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_TOP_RIGHT_CORNER:
        case mFM_BLOCK_TYPE_RIVER_EAST_CLIFF_TOP_RIGHT_CORNER:
        case mFM_BLOCK_TYPE_RIVER_EAST_CLIFF_TOP_RIGHT_CORNER_BRIDGE:
        case mFM_BLOCK_TYPE_RIVER_WEST_CLIFF_TOP_RIGHT_CORNER:
        case mFM_BLOCK_TYPE_RIVER_WEST_CLIFF_TOP_RIGHT_CORNER_BRIDGE:
        case mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_TOP_RIGHT_CORNER_BRIDGE:
            base = PREV_PAL_CLIFF;
            dir = 3;
            break;
        case mFM_BLOCK_TYPE_WATERFALL_STRAIGHT_CLIFF_TOP_LEFT_CORNER:
        case mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_BOTTOM_LEFT_CORNER:
        case mFM_BLOCK_TYPE_RIVER_STRAIGHT_CLIFF_BOTTOM_LEFT_CORNER_BRIDGE:
        case mFM_BLOCK_TYPE_RIVER_EAST_CLIFF_TOP_LEFT_CORNER:
        case mFM_BLOCK_TYPE_RIVER_EAST_CLIFF_TOP_LEFT_CORNER_BRIDGE:
        case mFM_BLOCK_TYPE_RIVER_WEST_CLIFF_TOP_LEFT_CORNER:
        case mFM_BLOCK_TYPE_RIVER_WEST_CLIFF_TOP_LEFT_CORNER_BRIDGE:
        case mFM_BLOCK_TYPE_WATERFALL_WEST_CLIFF_BOTTOM_LEFT_CORNER:
        case mFM_BLOCK_TYPE_WATERFALL_EAST_CLIFF_BOTTOM_RIGHT_CORNER:
            base = PREV_PAL_CLIFF;
            dir = 4;
            break;

        /* Pure cliffs */
        case mFM_BLOCK_TYPE_CLIFF_HORIZONTAL:
        case mFM_BLOCK_TYPE_CLIFF_BOTTOM_RIGHT_CORNER:
        case mFM_BLOCK_TYPE_CLIFF_VERTICAL_RIGHT:
        case mFM_BLOCK_TYPE_CLIFF_TOP_RIGHT_CORNER:
        case mFM_BLOCK_TYPE_CLIFF_TOP_LEFT_CORNER:
        case mFM_BLOCK_TYPE_CLIFF_VERTICAL_LEFT:
        case mFM_BLOCK_TYPE_CLIFF_BOTTOM_LEFT_CORNER:
            base = PREV_PAL_CLIFF;
            break;

        /* Slopes */
        case mFM_BLOCK_TYPE_SLOPE_HORIZONTAL:
        case mFM_BLOCK_TYPE_SLOPE_BOTTOM_RIGHT_CORNER:
        case mFM_BLOCK_TYPE_SLOPE_VERTICAL_RIGHT:
        case mFM_BLOCK_TYPE_SLOPE_TOP_RIGHT_CORNER:
        case mFM_BLOCK_TYPE_SLOPE_TOP_LEFT_CORNER:
        case mFM_BLOCK_TYPE_SLOPE_VERTICAL_LEFT:
        case mFM_BLOCK_TYPE_SLOPE_BOTTOM_LEFT_CORNER:
            base = PREV_PAL_SLOPE;
            break;

        /* Beach */
        case mFM_BLOCK_TYPE_BEACH:
        case mFM_BLOCK_TYPE_BEACH_RIVER:
        case mFM_BLOCK_TYPE_BEACH_RIVER_BRIDGE:
            base = PREV_PAL_SAND;
            break;

        /* Ocean / water */
        case mFM_BLOCK_TYPE_OCEAN:
        case mFM_BLOCK_TYPE_OCEAN_2:
        case mFM_BLOCK_TYPE_OCEAN_3:
        case mFM_BLOCK_TYPE_OCEAN_4:
        case mFM_BLOCK_TYPE_OCEAN_5:
        case mFM_BLOCK_TYPE_OCEAN_6:
        case mFM_BLOCK_TYPE_OCEAN_7:
        case mFM_BLOCK_TYPE_OCEAN_8:
        case mFM_BLOCK_TYPE_ISLAND_LEFT:
        case mFM_BLOCK_TYPE_ISLAND_RIGHT:
        case mFM_BLOCK_TYPE_SEA_EXCEPTIONAL:
        case mFM_BLOCK_TYPE_PORT:
            base = PREV_PAL_OCEAN;
            break;

        /* Pools */
        case mFM_BLOCK_TYPE_POOL_SOUTH:
            base = PREV_PAL_GRASS;
            stripe = PREV_PAL_POOL;
            dir = 1;
            break;
        case mFM_BLOCK_TYPE_POOL_EAST:
        case mFM_BLOCK_TYPE_POOL_WEST:
            base = PREV_PAL_GRASS;
            stripe = PREV_PAL_POOL;
            dir = 2;
            break;
        case mFM_BLOCK_TYPE_POOL_SOUTH_EAST:
        case mFM_BLOCK_TYPE_POOL_EAST_SOUTH:
            base = PREV_PAL_GRASS;
            stripe = PREV_PAL_POOL;
            dir = 3;
            break;
        case mFM_BLOCK_TYPE_POOL_SOUTH_WEST:
        case mFM_BLOCK_TYPE_POOL_WEST_SOUTH:
            base = PREV_PAL_GRASS;
            stripe = PREV_PAL_POOL;
            dir = 4;
            break;

        /* Tracks */
        case mFM_BLOCK_TYPE_TRACKS:
        case mFM_BLOCK_TYPE_TRACKS_STATION:
        case mFM_BLOCK_TYPE_TRACKS_DUMP:
        case mFM_BLOCK_TYPE_TRACKS_RIVER:
        case mFM_BLOCK_TYPE_TRACKS_RIVER_BRIDGE:
        case mFM_BLOCK_TYPE_TRACKS6:
        case mFM_BLOCK_TYPE_TRACKS7:
        case mFM_BLOCK_TYPE_TRACKS8:
        case mFM_BLOCK_TYPE_TRACKS9:
            base = PREV_PAL_TRACKS;
            break;

        /* Buildings */
        case mFM_BLOCK_TYPE_TRACKS_SHOP:
        case mFM_BLOCK_TYPE_TRACKS_POST_OFFICE:
        case mFM_BLOCK_TYPE_SHRINE:
        case mFM_BLOCK_TYPE_POLICE_BOX:
        case mFM_BLOCK_TYPE_MUSEUM:
        case mFM_BLOCK_TYPE_NEEDLEWORK:
            base = PREV_PAL_BUILDING;
            break;

        /* Border cliffs (shouldn't appear in FG blocks but handle gracefully) */
        default:
            base = PREV_PAL_DARK_GREEN;
            break;
    }

    mMP_fillPreviewBlock(dest, base, stripe, dir);
}
#endif

static int mMP_check_layer(f32 y) {
    int layer;

    switch (mCoBG_Height2GetLayer(y)) {
        case mCoBG_LAYER0:
            layer = 2;
            break;
        case mCoBG_LAYER1:
            layer = 1;
            break;
        case mCoBG_LAYER2:
            layer = 0;
            break;
        default:
            layer = 1;
            break;
    }

    if (mRF_CheckFieldStep3() == FALSE) {
        layer--;
    }

    if (layer < 0) {
        layer = 0;
    }

    return layer;
}

static int mMP_check_bg_kind(u8 kind) {
    int res = TRUE;
    if (kind <= mFM_BLOCK_TYPE_BORDER_CLIFF_CORNER_TOP_RIGHT || kind == mFM_BLOCK_TYPE_BORDER_CLIFF_LEFT_TRANSITION ||
        kind == mFM_BLOCK_TYPE_BORDER_CLIFF_RIGHT_TRANSITION || kind == mFM_BLOCK_TYPE_BORDER_CLIFF_LEFT_TUNNEL ||
        kind == mFM_BLOCK_TYPE_BORDER_CLIFF_RIGHT_TUNNEL) {
        res = FALSE;
    }

    return res;
}

static void mMP_make_max_no_table(int* max_no_table, int count) {
    int bz;
    int i;
    int j = 0;

    for (i = 0; i < count; i++) {
        max_no_table[i] = mFM_BLOCK_TYPE_FLAT;
    }

    // i = BLOCKXZ_2_BLOCKIDX(0, 1);
    for (bz = 1; bz < mMP_BLOCKZ_MAX; bz++) {
        u8 type;
        int bx;
        for (bx = 1; bx < mMP_BLOCKX_MAX; bx++) {
            type = g_block_type_p[BLOCKXZ_2_BLOCKIDX(bx, bz)];
            if (mMP_check_bg_kind(type) == TRUE) {
                max_no_table[0] = type;

                if (Save_Get(bridge.exists) && Save_Get(bridge.block_x) == bx && Save_Get(bridge.block_z) == bz &&
                    pluss_bridge[type] != 0xFF) {
                    max_no_table[0] = pluss_bridge[type];
                }

                max_no_table++;
                j++;
            }

            if (j >= count) {
                return;
            }
        }
    }
}

static void mMP_make_max_no_table_select(int* max_no_table, int count, mFM_combination_c* combi_table) {
    int i;
    int j = 0;
    int bz;
    int bx;

    for (i = 0; i < count; i++) {
        max_no_table[i] = mFM_BLOCK_TYPE_FLAT;
    }

    for (bz = 1; bz < mMP_BLOCKZ_MAX; bz++) {
        for (bx = 1; bx < FG_BLOCK_X_NUM + 1; bx++) {
            u16 raw = *(u16*)&combi_table[bz * (FG_BLOCK_X_NUM + 2) + bx];
            int combo_type = (raw >> 2) & 0x3FFF;
            u8 type = data_combi_table[combo_type].type;

            if (mMP_check_bg_kind(type) == TRUE) {
                max_no_table[0] = type;
                max_no_table++;
                j++;
            }

            if (j >= count) {
                return;
            }
        }
    }
}

static void mMP_dma_texture(u8** texture_pp, int type) {
    *texture_pp = l_map_texture[type];
}

static void mMP_set_map_texture_pal(mMP_Ovl_c* map_ovl) {
    int max_no_table[FG_BLOCK_TOTAL_NUM];
    int i;

    mMP_make_max_no_table(max_no_table, FG_BLOCK_TOTAL_NUM);
    for (i = 0; i < FG_BLOCK_TOTAL_NUM; i++) {
        map_ovl->map_pal[i] = l_map_pal[max_no_table[i]];
        mMP_dma_texture(&map_ovl->map_texture[i], max_no_table[i]);
    }
}

static void mMP_set_map_texture_pal_select(mMP_Ovl_c* map_ovl, mFM_combination_c* combi_table) {
    int max_no_table[FG_BLOCK_TOTAL_NUM];
    int i;

    mMP_make_max_no_table_select(max_no_table, FG_BLOCK_TOTAL_NUM, combi_table);
    for (i = 0; i < FG_BLOCK_TOTAL_NUM; i++) {
        map_ovl->map_pal[i] = l_map_pal[max_no_table[i]];
        mMP_dma_texture(&map_ovl->map_texture[i], max_no_table[i]);
    }
}

static mActor_name_t mMP_GetFgBlockName(mMP_Ovl_c* map_ovl, int block_x, int block_z) {
    return map_ovl->combination_table[Save_Get(combi_table[block_z][block_x]).combination_type].fg_id;
}

static void mMP_set_house_data(mMP_Ovl_c* map_ovl, mSM_MenuInfo_c* menu) {
    static u8 akiya_str[PLAYER_NAME_LEN] = "free    ";

    mMP_ResidentInfo_c* resident_p;
    mMP_LabelInfo_c* block_label;
    mMP_HousePos_c* house_pos;
    mMP_HousePos_c* house_pos_top;
    mMP_HousePos_Entry_c* pos_info;
    Private_c* priv;
    Animal_c* animal;
    Anmhome_c* home;
    int i;
    int j;
    int residents;
    mActor_name_t fgblock_name;

    priv = Save_Get(private_data);
    animal = Save_Get(animals);
    resident_p = &map_ovl->player_info[0];
    block_label = &map_ovl->label_info[1][2];

    /* Process player resident info first */
    block_label->label_cnt = PLAYER_NUM;
    block_label->label_no = mMP_LABEL_PLAYER;

    residents = 0;
    for (i = 0; i < PLAYER_NUM; i++, priv++) {
        if (mPr_CheckPrivate(priv) == TRUE && (Common_Get(now_private) != priv || mEv_CheckFirstIntro() == FALSE)) {
            mPr_CopyPlayerName(resident_p->name, priv->player_ID.player_name);
            resident_p->sex = priv->gender;
            resident_p->house_layer = 0;
            resident_p->house_idx = 0;

            block_label->residents[residents] = resident_p;
            residents++;
            resident_p++;
        }
    }

    for (residents; residents < PLAYER_NUM; residents++, resident_p++) {
        mPr_CopyPlayerName(resident_p->name, akiya_str);
        resident_p->sex = -1;
        resident_p->house_layer = 0;

        block_label->residents[residents] = resident_p;
    }

    house_pos_top = mMP_house_pos_list;
    /* Process animal resident info next */
    resident_p = &map_ovl->animal_info[0];
    residents = 0;

    for (i = 0; i < ANIMAL_NUM_MAX; i++, animal++, resident_p++) {
        if (mNpc_CheckFreeAnimalInfo(animal) == FALSE) {
            home = &animal->home_info;
            mNpc_GetNpcWorldNameAnm(resident_p->name, &animal->id);
            resident_p->sex = mNpc_GetLooks2Sex(animal->id.looks);
            resident_p->house_layer = mMP_check_layer(Common_Get(npclist[i].house_position.y));

            fgblock_name = mMP_GetFgBlockName(map_ovl, home->block_x, home->block_z);
            house_pos = house_pos_top;
            pos_info = house_pos[0].entries;

            for (house_pos; house_pos->fgblock_name != 0x03B8; house_pos++) {
                if (fgblock_name == house_pos->fgblock_name) {
                    pos_info = house_pos->entries;

                    for (j = 0; j < 3; j++, pos_info++) {
                        if (pos_info->ut_x == home->ut_x && pos_info->ut_z == home->ut_z - 1) {
                            break;
                        }
                    }

                    if (j == 3) {
                        pos_info = &house_pos->entries[0]; // default
                    }

                    break;
                }
            }

            resident_p->house_idx = pos_info->idx;

            block_label = &map_ovl->label_info[home->block_z - 1][home->block_x - 1];
            block_label->residents[block_label->label_cnt] = resident_p;
            block_label->label_cnt++;
            block_label->label_no = mMP_LABEL_NPC;
        }
    }

    /* Sort villager houses by layer, with lower layers coming first */
    block_label = (mMP_LabelInfo_c*)map_ovl->label_info;
    for (i = 0; i < FG_BLOCK_TOTAL_NUM; i++, block_label++) {
        if (block_label->label_no == mMP_LABEL_NPC && block_label->label_cnt > 1) {
            for (j = 0; j < block_label->label_cnt - 1; j++) {
                if (block_label->residents[j]->house_layer > block_label->residents[j + 1]->house_layer) {
                    resident_p = block_label->residents[j];
                    block_label->residents[j] = block_label->residents[j + 1];
                    block_label->residents[j + 1] = resident_p;
                    j = -1;
                }
            }
        }
    }
}

static void mMP_set_field_data(mMP_Ovl_c* map_ovl) {
    mMP_LabelInfo_c* label_info = (mMP_LabelInfo_c*)map_ovl->label_info;
    int* block_kind_p = g_block_kind_p + BLOCKXZ_2_BLOCKIDX(1, 1); // start at the first non-border acre
    int bx;
    int bz;

    for (bz = 0; bz < FG_BLOCK_Z_NUM; bz++) {
        for (bx = 0; bx < FG_BLOCK_X_NUM; bx++) {
            if (label_info->label_cnt <= 0) {
                int kind = *block_kind_p;

                if (kind & mRF_BLOCKKIND_SHOP) {
                    label_info->label_no = mMP_LABEL_SHOP;
                    label_info->label_cnt = 2;
                } else if (kind & mRF_BLOCKKIND_POLICE) {
                    label_info->label_no = mMP_LABEL_POLICE;
                    label_info->label_cnt = 2;
                } else if (kind & mRF_BLOCKKIND_POSTOFFICE) {
                    label_info->label_no = mMP_LABEL_POST;
                    label_info->label_cnt = 2;
                } else if (kind & mRF_BLOCKKIND_SHRINE) {
                    label_info->label_no = mMP_LABEL_SHRINE;
                    label_info->label_cnt = 2;
                } else if (kind & mRF_BLOCKKIND_STATION) {
                    label_info->label_no = mMP_LABEL_STATION;
                    label_info->label_cnt = 2;
                } else if (kind & mRF_BLOCKKIND_DUMP) {
                    label_info->label_no = mMP_LABEL_JUNK;
                    label_info->label_cnt = 2;
                } else if (kind & mRF_BLOCKKIND_MUSEUM) {
                    label_info->label_no = mMP_LABEL_MUSEUM;
                    label_info->label_cnt = 2;
                } else if (kind & mRF_BLOCKKIND_TAILORS) {
                    label_info->label_no = mMP_LABEL_NEEDLE;
                    label_info->label_cnt = 2;
                } else if (kind & mRF_BLOCKKIND_DOCK) {
                    label_info->label_no = mMP_LABEL_PORT;
                    label_info->label_cnt = 2;
                }
            }

            label_info++;
            block_kind_p++;
        }

        block_kind_p += 2; // skip X border acres
    }
}

static void mMP_set_init_data(mMP_Ovl_c* map_ovl, mSM_MenuInfo_c* menu_info) {
    mMP_LabelInfo_c* label_info = (mMP_LabelInfo_c*)map_ovl->label_info;
    PLAYER_ACTOR* player = get_player_actor_withoutCheck((GAME_PLAY*)gamePT);
    int block_x;
    int block_z;
    int i;

    if (Common_Get(field_type) != mFI_FIELDTYPE2_FG && Common_Get(door_data.next_scene_id) != 0) {
        xyz_t pos;

        xyz_t_move_s_xyz(&pos, Common_GetPointer(structure_exit_door_data.exit_position));
        mFI_Wpos2BlockNum(&block_x, &block_z, pos);
    } else {
        mFI_Wpos2BlockNum(&block_x, &block_z, player->actor_class.world.position);
    }

    map_ovl->player_bx = block_x - 1;
    map_ovl->player_bz = block_z - 1;

    map_ovl->sel_bx = map_ovl->player_bx;
    map_ovl->sel_bz = map_ovl->player_bz;

    map_ovl->map_cursor_current_xpos = (f32)map_ovl->player_bx * mMP_BLOCK_SIZE_F;
    map_ovl->map_cursor_current_zpos = -(f32)map_ovl->player_bz * mMP_BLOCK_SIZE_F;

    map_ovl->cursor_frame = 0;

    map_ovl->map_cursor_target_xpos = map_ovl->map_cursor_current_xpos;
    map_ovl->map_cursor_target_zpos = map_ovl->map_cursor_current_zpos;

    mMP_set_map_texture_pal(map_ovl);

    for (i = 0; i < FG_BLOCK_TOTAL_NUM; i++, label_info++) {
        label_info->label_cnt = 0;
    }

    mMP_set_house_data(map_ovl, menu_info);
    mMP_set_field_data(map_ovl);

    map_ovl->land_name_str_len = mMl_strlen(Save_Get(land_info.name), LAND_NAME_SIZE, CHAR_SPACE);
}

static void mMP_move_Move(Submenu* submenu, mSM_MenuInfo_c* menu) {
    (*submenu->overlay->move_Move_proc)(submenu, menu);
}

static void mMP_move_Play(Submenu* submenu, mSM_MenuInfo_c* menu) {
    Submenu_Overlay_c* ovl = submenu->overlay;
    int trigger = ovl->menu_control.trigger;
    mMP_Ovl_c* map_ovl = ovl->map_ovl;
    int cursor_moved = FALSE;
    int exit_map = FALSE;

#if VERSION >= VER_DELUXE
    if (menu->data0 == 2) {
        /* Preview mode: A=accept, B=next layout */
        if (trigger & BUTTON_A) {
            menu->data1 = 0; /* accepted */
            exit_map = TRUE;
        } else if (trigger & BUTTON_B) {
            menu->data1 = 1; /* rejected, show next */
            exit_map = TRUE;
        }
    } else
#endif
    if ((trigger & (BUTTON_A | BUTTON_B | BUTTON_START)) || (menu->data0 == 1 && (trigger & BUTTON_X))) {
        exit_map = TRUE;
    }

    if (exit_map) {
        (*ovl->move_chg_base_proc)(menu, mSM_MOVE_OUT_TOP);
        sAdo_SysTrgStart(0x17D);
    } else if (trigger & BUTTON_CLEFT) {
        if (map_ovl->sel_bx != 0) {
            map_ovl->sel_bx--;
            map_ovl->map_cursor_target_xpos -= mMP_BLOCK_SIZE_F;
            cursor_moved = TRUE;
        }
    } else if (trigger & BUTTON_CDOWN) {
        if (map_ovl->sel_bz < FG_BLOCK_Z_NUM - 1) {
            map_ovl->sel_bz++;
            map_ovl->map_cursor_target_zpos -= mMP_BLOCK_SIZE_F;
            cursor_moved = TRUE;
        }
    } else if (trigger & BUTTON_CUP) {
        if (map_ovl->sel_bz != 0) {
            map_ovl->sel_bz--;
            map_ovl->map_cursor_target_zpos += mMP_BLOCK_SIZE_F;
            cursor_moved = TRUE;
        }
    } else if (trigger & BUTTON_CRIGHT) {
        if (map_ovl->sel_bx < FG_BLOCK_X_NUM - 1) {
            map_ovl->sel_bx++;
            map_ovl->map_cursor_target_xpos += mMP_BLOCK_SIZE_F;
            cursor_moved = TRUE;
        }
    }

    if (cursor_moved == TRUE) {
        menu->proc_status = 2; // TODO: enum & name
        sAdo_SysTrgStart(NA_SE_CURSOL);
    }

    map_ovl->cursor_frame++;
    if (map_ovl->cursor_frame >= mMP_CURSOR_FRAMES) {
        map_ovl->cursor_frame %= mMP_CURSOR_FRAMES;
    } else if (map_ovl->cursor_frame < 0) {
        map_ovl->cursor_frame = 0;
    }
}

static void mMP_move_Wait(Submenu* submenu, mSM_MenuInfo_c* menu) {
    mMP_Ovl_c* map_ovl = submenu->overlay->map_ovl;
    f32 x_remain = add_calc(&map_ovl->map_cursor_current_xpos, map_ovl->map_cursor_target_xpos, 0.7f, 19.0f, 1.8f);
    f32 z_remain = add_calc(&map_ovl->map_cursor_current_zpos, map_ovl->map_cursor_target_zpos, 0.7f, 19.0f, 1.8f);

    if (fabsf(x_remain) < 0.1f && fabsf(z_remain) < 0.1f) {
        menu->proc_status = 1;
    }
}

static void mMP_move_End(Submenu* submenu, mSM_MenuInfo_c* menu) {
    (*submenu->overlay->move_End_proc)(submenu, menu);
}

typedef void (*mMP_MOVE_PROC)(Submenu*, mSM_MenuInfo_c*);
static void mMP_map_ovl_move(Submenu* submenu) {
    static mMP_MOVE_PROC ovl_move_proc[5] = { &mMP_move_Move, &mMP_move_Play, &mMP_move_Wait,
                                              (mMP_MOVE_PROC)&none_proc1, &mMP_move_End };

    Submenu_Overlay_c* ovl = submenu->overlay;
    mSM_MenuInfo_c* menu = &ovl->menu_info[mSM_OVL_MAP];

    (*menu->pre_move_func)(submenu);
    (*ovl_move_proc[menu->proc_status])(submenu, menu);
}

static void mMP_map_draw_init() {
    kan_win_wakuT_model_p = kan_win_wakuT_model;
    kan_win_waku1T_model_p = kan_waku_w1T_model;
    kan_win_waku2T_model_p = kan_waku_w2T_model;
    kan_win_waku3T_model_p = kan_waku_w3T_model;
    kan_win_waku4T_model_p = kan_waku_w4T_model;
    kan_win_color0_mode_p = kan_win_color0_mode;
    kan_win_color1_mode_p = kan_win_color1_mode;
}

static int mMP_get_label_cnt(mMP_LabelInfo_c* label_info) {
    int label_no = label_info->label_no;
    if (label_no == mMP_LABEL_NPC || label_no == mMP_LABEL_PLAYER) {
        return label_info->label_cnt;
    }

    return 1;
}

static void mMP_set_house_dl(GRAPH* graph, mMP_Ovl_c* map_ovl, f32 pos_x, f32 pos_y) {
    static f32 offset_x[3] = { 5.0f, 13.0f, 17.0f };
    static f32 offset_y[3] = { -4.0f, -11.0f, -18.0f };

    mMP_LabelInfo_c* label_info;
    f32 y;
    f32 base_x;
    f32 temp_x;
    f32 base_y;
    mMP_ResidentInfo_c** resident_p;
    int bz;
    int bx;
    int i;
    Gfx* gfx;

    label_info = (mMP_LabelInfo_c*)map_ovl->label_info;

    base_y = (pos_y + 45.7f) + (0.5f * mMP_BLOCK_SIZE_F);
    base_x = (pos_x + 11.7f) - (0.5f * mMP_BLOCK_SIZE_F);

    OPEN_DISP(graph);

    gfx = NOW_POLY_OPA_DISP;
    gSPDisplayList(gfx++, kan_win_mode);

    /* draw all villager houses */

    for (bz = 0; bz < FG_BLOCK_Z_NUM; bz++) {
        temp_x = (pos_x + 11.7f) - (0.5f * mMP_BLOCK_SIZE_F);
        for (bx = 0; bx < FG_BLOCK_X_NUM; bx++) {
            if (label_info->label_no == mMP_LABEL_NPC) {
                resident_p = label_info->residents;

                for (i = 0; i < label_info->label_cnt; i++) {
                    Matrix_scale(16.0f, 16.0f, 1.0f, MTX_LOAD);
                    Matrix_translate(temp_x + offset_x[(*resident_p)->house_idx % 3],
                                     base_y + offset_y[(*resident_p)->house_idx / 3], 140.0f, MTX_MULT);

                    gSPMatrix(gfx++, _Matrix_to_Mtx_new(graph), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
                    gSPDisplayList(gfx++, kan_win_npc2T_table[(*resident_p)->house_layer]);

                    resident_p++;
                }
            }

            temp_x += mMP_BLOCK_SIZE_F;
            label_info++;
        }

        base_y -= mMP_BLOCK_SIZE_F;
        // base_x = x_save;
    }

    /* draw "current acre" icon */

    Matrix_scale(16.0f, 16.0f, 1.0f, MTX_LOAD);
    Matrix_translate(
        ((pos_x + 11.7f) - (0.5f * mMP_BLOCK_SIZE_F) + (f32)map_ovl->player_bx * mMP_BLOCK_SIZE_F) + 3.0f + 5.0f,
        (pos_y + 45.7f - (0.5f * mMP_BLOCK_SIZE_F) - (f32)map_ovl->player_bz * mMP_BLOCK_SIZE_F) + 4.0f + 5.0f, 140.0f,
        1);

    gSPMatrix(gfx++, _Matrix_to_Mtx_new(graph), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPDisplayList(gfx++, kan_win_genzaiT_model);

    SET_POLY_OPA_DISP(gfx);
    CLOSE_DISP(graph);
}

static void mMP_set_map_dl(GRAPH* graph, mMP_Ovl_c* map_ovl) {
    int bz;
    int bx;
    int block = 0;
    Gfx* gfx;

    OPEN_DISP(graph);

    gfx = NOW_POLY_OPA_DISP;

    gDPSetTextureAdjustMode(gfx++, G_TA_DOLPHIN);
    gSPDisplayList(gfx++, kan_tizu_mode);

    Matrix_push();
    Matrix_translate(11.7f, 45.7f, 0.0f, MTX_MULT);

    for (bz = 0; bz < FG_BLOCK_Z_NUM; bz++) {
        for (bx = 0; bx < FG_BLOCK_X_NUM; bx++) {
            gSPSegment(gfx++, ANIME_1_TXT_SEG, map_ovl->map_texture[block]);
            gSPSegment(gfx++, ANIME_2_TXT_SEG, l_kan_tizu_pal[map_ovl->map_pal[block]]);
            gSPMatrix(gfx++, _Matrix_to_Mtx_new(graph), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(gfx++, kan_tizu_model);

            /* move right one acre */
            Matrix_translate(mMP_BLOCK_SIZE_F, 0.0f, 0.0f, MTX_MULT);
            block++;
        }

        /* move left to '1' acre and down one acre */
        Matrix_translate((-mMP_BLOCK_SIZE_F) * FG_BLOCK_X_NUM, -mMP_BLOCK_SIZE_F, 0.0f, MTX_MULT);
    }

    Matrix_pull();
    gDPSetTextureAdjustMode(gfx++, G_TA_N64);

    SET_POLY_OPA_DISP(gfx);
    CLOSE_DISP(graph);
}

#if 0 /* VER_DELUXE structure models not available */
static void mMP_set_map_block_preview_structure_dl(GRAPH* graph) {
    Gfx* gfx;
    int i;
    f32 ut_size_x = mMP_BLOCK_SIZE_F / (f32)UT_X_NUM;
    f32 ut_size_z = mMP_BLOCK_SIZE_F / (f32)UT_Z_NUM;
    f32 half_block = 0.5f * mMP_BLOCK_SIZE_F;
    f32 z_depth = 140.0f;
    f32 scale_val = 16.0f;

    OPEN_DISP(graph);
    gfx = NOW_POLY_OPA_DISP;

    gSPDisplayList(gfx++, kan_win_mode);

    Matrix_push();

    for (i = 0; i < structure_num; i++) {
        mMP_StructurePosEntry_c* pos = &structure_pos[i];
        const mMP_StructureInfoEntry_c* info;
        f32 x;
        f32 y;

        info = &structure_info[pos->item];

        x = (f32)pos->ut_x * ut_size_x + info->ofs_x;
        y = -(f32)pos->ut_z * ut_size_z + info->ofs_z;

        Matrix_push();
        Matrix_translate(x + 11.7f, y + 45.7f - half_block, z_depth, MTX_MULT);
        Matrix_scale(scale_val, scale_val, scale_val, MTX_MULT);

        gSPMatrix(gfx++, _Matrix_to_Mtx_new(graph), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPDisplayList(gfx++, info->gfx);

        Matrix_pull();
    }

    Matrix_pull();
    SET_POLY_OPA_DISP(gfx);
    CLOSE_DISP(graph);
}
#endif

#if VERSION >= VER_DELUXE
static void mMP_set_map_block_preview_dl(GRAPH* graph, mMP_Ovl_c* map_ovl) {
    int bz;
    int bx;
    int block = 0;
    Gfx* gfx;

    OPEN_DISP(graph);

    gfx = NOW_POLY_OPA_DISP;

    gDPSetTextureAdjustMode(gfx++, G_TA_DOLPHIN);
    gSPDisplayList(gfx++, kan_tizu_mode);

    Matrix_push();
    Matrix_translate(11.7f, 45.7f, 0.0f, MTX_MULT);

    for (bz = 0; bz < FG_BLOCK_Z_NUM; bz++) {
        for (bx = 0; bx < FG_BLOCK_X_NUM; bx++) {
            gSPSegment(gfx++, ANIME_1_TXT_SEG, &block_preview_textures[block * MAP_PREVIEW_TEX_SIZE]);
            gSPSegment(gfx++, ANIME_2_TXT_SEG, preview_pal_colors);
            gSPMatrix(gfx++, _Matrix_to_Mtx_new(graph), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(gfx++, kan_tizu_model);

            Matrix_translate(mMP_BLOCK_SIZE_F, 0.0f, 0.0f, MTX_MULT);
            block++;
        }

        Matrix_translate((-mMP_BLOCK_SIZE_F) * FG_BLOCK_X_NUM, -mMP_BLOCK_SIZE_F, 0.0f, MTX_MULT);
    }

    Matrix_pull();
    gDPSetTextureAdjustMode(gfx++, G_TA_N64);

    gSPEndDisplayList(gfx++);

    SET_POLY_OPA_DISP(gfx);

    CLOSE_DISP(graph);
}
#endif

static void mMP_set_cursol_dl(GRAPH* graph, mMP_Ovl_c* map_ovl, f32 base_x, f32 base_y) {
    static int col_g[mMP_CURSOR_FRAMES] = { 0, 1, 2, 5, 10, 20, 50, 75, 90, 100, 90, 75, 50, 20, 10, 5, 2, 1 };

    static f32 cursol_scale[mMP_CURSOR_FRAMES] = { 1.0f, 1.015f, 1.03f, 1.04f, 1.05f, 1.06f, 1.07f, 1.08f, 1.09f,
                                                   1.1f, 1.09f,  1.08f, 1.07f, 1.06f, 1.05f, 1.04f, 1.03f, 1.015f };

    f32 x = map_ovl->map_cursor_current_xpos + base_x;
    f32 y = map_ovl->map_cursor_current_zpos + base_y;
    int frame = map_ovl->cursor_frame;
    Gfx* gfx;

    Matrix_scale(16.0f, 16.0f, 1.0f, MTX_LOAD);
    Matrix_translate(x + 11.7f, y + 45.7f, 140.0f, MTX_MULT);

    Matrix_scale(cursol_scale[frame], cursol_scale[frame], 1.0f, MTX_MULT);

    OPEN_DISP(graph);

    gfx = NOW_POLY_OPA_DISP;

    gSPMatrix(gfx++, _Matrix_to_Mtx_new(graph), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gDPSetPrimColor(gfx++, 0, 255, 255, col_g[frame], 255, 255);
    gSPDisplayList(gfx++, kan_win_cursorT_model);

    SET_POLY_OPA_DISP(gfx);

    CLOSE_DISP(graph);
}

static void mMP_set_label_word_dl(GAME* game, u8* str, int str_len, const rgb8_t* color, f32 x, f32 y, f32 scale_x) {
    mFont_SetLineStrings(game, str, str_len, x, y, color->r, color->g, color->b, 255, TRUE, FALSE, scale_x, 0.75f,
                         mFont_MODE_POLY);
}

static void mMP_set_label_building_dl(GAME* game, mMP_LabelWord_c* word, f32 x, f32 y) {
    static const rgb8_t building_color = { 120, 95, 205 };

    mMP_set_label_word_dl(game, word->str, word->str_len, &building_color, x, y, 0.75f);
}

static void mMP_set_label_resident_dl(GAME* game, mMP_LabelInfo_c* label_info, mMP_ResidentInfo_c** resident_p,
                                      f32 x, f32 y) {
    static const rgb8_t sex_color[3] = {
        { 60, 155, 225 },
        { 255, 105, 210 },
        { 255, 75, 40 },
    };
    static const rgb8_t unoccupied_color = { 165, 145, 140 };

    f32 scale_max = 0.75f;
    f32 width_max = 54.0f;
    int i;

    for (i = 0; i < label_info->label_cnt; i++) {
        int width;
        f32 scale_x;
        const rgb8_t* color;
        s8 sex;

        width = mFont_GetStringWidth((*resident_p)->name, PLAYER_NAME_LEN, TRUE);
        if (width <= 0) {
            scale_x = scale_max;
        } else {
            scale_x = width_max / (f32)width;
            if (scale_x > scale_max) {
                scale_x = scale_max;
            }
        }

        sex = (*resident_p)->sex;
        if (sex != -1) {
            color = &sex_color[sex];
        } else {
            color = &unoccupied_color;
        }

        mMP_set_label_word_dl(game, (*resident_p)->name, PLAYER_NAME_LEN, color, x, y, scale_x);

        y += 12.0f;
        resident_p++;
    }
}

static void mMP_set_label_dl(GAME* game, mMP_LabelInfo_c* label_info, f32 xpos, f32 ypos) {
    mMP_Label_c* label = mMP_label_data[label_info->label_no];

    if (label != NULL) {
        mMP_ResidentInfo_c** resident_p = label_info->residents;
        mMP_LabelWord_c* word;
        f32 base_x = 160.0f + xpos;
        f32 base_y = 120.0f - ypos;
        int i;

        for (i = 0; i < mMP_LABEL_WORD_NUM; i++) {
            word = label->words[i];

            if (word != NULL) {
                if (word->str == NULL) {
                    /* draw resident name label (villagers, players) */
                    int j;

                    for (j = 0; j < label_info->label_cnt; j++) {
                        int width = mFont_GetStringWidth((*resident_p)->name, PLAYER_NAME_LEN, TRUE);
                        f32 scale_x = 54.0f / width;

                        if (scale_x > 0.75f) {
                            scale_x = 0.75f;
                        }

                        if (resident_p[0]->sex != -1) {
                            mFont_SetLineStrings(game, resident_p[0]->name, PLAYER_NAME_LEN, base_x + word->ofs_x,
                                                 base_y - word->ofs_y, 255, 75, 40, 255, FALSE, TRUE, scale_x, 0.75f,
                                                 mFont_MODE_POLY);
                        } else {
                            mFont_SetLineStrings(game, resident_p[0]->name, PLAYER_NAME_LEN, base_x + word->ofs_x,
                                                 base_y - word->ofs_y, 165, 145, 140, 255, FALSE, TRUE, scale_x, 0.75f,
                                                 mFont_MODE_POLY);
                        }

                        resident_p++;
                        base_y += 12.0f;
                    }
                } else {
                    /* draw building name */
                    mFont_SetLineStrings(game, word->str, word->str_len, base_x + word->ofs_x, base_y - word->ofs_y,
                                         120, 95, 205, 255, FALSE, TRUE, 0.75f, 0.75f, mFont_MODE_POLY);
                }
            }
        }
    }
}

static void mMP_set_base_dl(GRAPH* graph, mSM_MenuInfo_c* menu) {
    Gfx* gfx;

    OPEN_DISP(graph);
    gfx = NOW_POLY_OPA_DISP;

    if (menu->data0 == 0) {
        gSPSegment(gfx++, G_MWO_SEGMENT_B, kan_win_color0_mode_p);
    } else {
        gSPSegment(gfx++, G_MWO_SEGMENT_B, kan_win_color1_mode_p);
    }

    gSPMatrix(gfx++, _Matrix_to_Mtx_new(graph), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPDisplayList(gfx++, kan_win_model);

    SET_POLY_OPA_DISP(gfx);
    CLOSE_DISP(graph);
}

static void mMP_update_acre_label_vertices(Vtx* vtx, int s, int t) {
    u16 s0 = (u16)(s * 32);
    u16 t0 = (u16)(t * 32);
    u16 s1 = (u16)((s + 32) * 32);
    u16 t1 = (u16)((t + 32) * 32);

    vtx[0].v.tc[0] = s0;
    vtx[0].v.tc[1] = t0;
    vtx[1].v.tc[0] = s0;
    vtx[1].v.tc[1] = t1;
    vtx[2].v.tc[0] = s1;
    vtx[2].v.tc[1] = t0;
    vtx[3].v.tc[0] = s1;
    vtx[3].v.tc[1] = t1;
}

static void mMP_set_win_dl(GRAPH* graph, mMP_Ovl_c* map_ovl, mMP_LabelInfo_c* label_info) {
    int label_count = label_info->label_cnt;
    Gfx* gfx;

    OPEN_DISP(graph);
    gfx = NOW_POLY_OPA_DISP;

    /* move X-acre number texture into segment 8 and Z-acre character texture int segment 9 */
    gSPSegment(gfx++, ANIME_1_TXT_SEG, kan_win_evw_anime_1_tex_table[map_ovl->sel_bx]);
    gSPSegment(gfx++, ANIME_2_TXT_SEG, kan_win_evw_anime_2_tex_table[map_ovl->sel_bz]);

    /* move label area texture into segment 10 based on number of labels */
    if (label_count == 4) {
        gSPSegment(gfx++, ANIME_3_TXT_SEG, kan_win_waku4T_model_p);
    } else if (label_count == 3) {
        gSPSegment(gfx++, ANIME_3_TXT_SEG, kan_win_waku3T_model_p);
    } else if (label_count == 1) {
        gSPSegment(gfx++, ANIME_3_TXT_SEG, kan_win_wakuT_model_p);
    } else if (label_count == 0) {
        gSPSegment(gfx++, ANIME_3_TXT_SEG, kan_win_waku1T_model_p);
    } else { /* label_count == 2 */
        gSPSegment(gfx++, ANIME_3_TXT_SEG, kan_win_waku2T_model_p);
    }

    gSPMatrix(gfx++, _Matrix_to_Mtx_new(graph), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPDisplayList(gfx++, kan_win_model2);

    SET_POLY_OPA_DISP(gfx);
    CLOSE_DISP(graph);
}

static void mMP_set_label_top_dl(GRAPH* graph, mMP_LabelInfo_c* label_info) {
    mMP_Label_c* label = mMP_label_data[label_info->label_no];
    mMP_ResidentInfo_c** resident_p;

    if (label != NULL) {
        int count = mMP_get_label_cnt(label_info);
        int i;
        Gfx* gfx;

        OPEN_DISP(graph);
        gfx = NOW_POLY_OPA_DISP;

        Matrix_push();
        Matrix_translate(label->ofs_x, label->ofs_y, 0.0f, MTX_MULT);

        if (label_info->label_no == mMP_LABEL_NPC) {
            resident_p = label_info->residents;
        } else {
            resident_p = NULL;
        }

        for (i = 0; i < count; i++) {
            gSPMatrix(gfx++, _Matrix_to_Mtx_new(graph), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

            if (resident_p != NULL && *resident_p != NULL && (*resident_p)->house_layer != 0) {
                gSPDisplayList(gfx++, kan_win_npcT_table[(*resident_p)->house_layer]);
                resident_p++;
            } else {
                gSPDisplayList(gfx++, label->gfx);

                if (resident_p != NULL) {
                    resident_p++;
                }
            }

            Matrix_translate(0.0f, -12.0f, 0.0f, MTX_MULT);
        }

        Matrix_pull();

        SET_POLY_OPA_DISP(gfx);
        CLOSE_DISP(graph);
    }
}

static void mMP_set_dl(Submenu* submenu, GAME* game, mSM_MenuInfo_c* menu) {
    static u8 land_color[3][4] = {
        { 255, 0, 255, 255 },
        { 60, 60, 255, 255 },
        { 255, 255, 255, 255 },
    };

    GRAPH* graph = game->graph;
    mMP_Ovl_c* map_ovl = submenu->overlay->map_ovl;
    int color_idx = menu->data0;
    u8* col;
    mMP_LabelInfo_c* label_info = &map_ovl->label_info[map_ovl->sel_bz][map_ovl->sel_bx];
    u8* land_name;
    f32 xpos = menu->position[0];
    f32 ypos = menu->position[1];
    f32 str_xpos;
    f32 str_ypos;

    if (color_idx > 2) {
        color_idx = 0;
    }
    col = land_color[color_idx];

    Matrix_scale(16.0f, 16.0f, 1.0f, MTX_LOAD);
    Matrix_translate(xpos, ypos, 140.0f, MTX_MULT);

    mMP_set_base_dl(graph, menu);
    mMP_set_map_dl(graph, map_ovl);
    mMP_set_win_dl(graph, map_ovl, label_info);
#if VERSION >= VER_DELUXE
    if (menu->data0 != 2)
#endif
    {
        mMP_set_label_top_dl(graph, label_info);
        mMP_set_house_dl(graph, map_ovl, xpos, ypos);
    }
    mMP_set_cursol_dl(graph, map_ovl, xpos, ypos);

    (*submenu->overlay->set_char_matrix_proc)(graph);

    str_xpos = (xpos + -136.0f) + 24.0f;
    str_ypos = (ypos + 102.0f) + -29.0f;

#if VERSION >= VER_DELUXE
    if (menu->data0 == 2) {
        /* Preview mode: show "Map N" and button hints instead of town name */
        static u8 map_str[] = { CHAR_M, CHAR_a, CHAR_p, CHAR_SPACE, CHAR_ONE };
        static u8 hint_str[] = {
            CHAR_A, CHAR_COLON, CHAR_S, CHAR_e, CHAR_l, CHAR_e, CHAR_c, CHAR_t,
            CHAR_SPACE, CHAR_SPACE,
            CHAR_B, CHAR_COLON, CHAR_N, CHAR_e, CHAR_x, CHAR_t
        };

        map_str[4] = CHAR_ONE + (u8)((menu->data3 & 3));
        /* Map counter in bright yellow at town name position */
        mFont_SetLineStrings(game, map_str, 5, str_xpos + 160.0f, 120.0f - str_ypos,
                             255, 255, 60, 255, FALSE, TRUE, 1.2f, 1.2f, mFont_MODE_POLY);
        /* Button hints below in white */
        mFont_SetLineStrings(game, hint_str, 16, str_xpos + 160.0f - 16.0f, 120.0f - str_ypos + 16.0f,
                             255, 255, 255, 220, FALSE, TRUE, 0.75f, 0.75f, mFont_MODE_POLY);
    } else
#endif
    {
        /* draw town name */
        land_name = Save_Get(land_info.name);
        mFont_SetLineStrings(game, land_name, map_ovl->land_name_str_len, str_xpos + 160.0f, 120.0f - str_ypos, col[0],
                             col[1], col[2], 255, FALSE, TRUE, 1.0f, 1.0f, mFont_MODE_POLY);
    }

#if VERSION >= VER_DELUXE
    if (menu->data0 != 2)
#endif
    {
        mMP_set_label_dl(game, label_info, xpos, ypos);
    }
}

static void mMP_map_ovl_draw(Submenu* submenu, GAME* game) {
    Submenu_Overlay_c* overlay = submenu->overlay;
    mSM_MenuInfo_c* menu = &overlay->menu_info[mSM_OVL_MAP];

    (*menu->pre_draw_func)(submenu, game);
    mMP_set_dl(submenu, game, menu);
}

#if VERSION >= VER_DELUXE
extern void mMP_map_ovl_init_block_preview(void) {
    int bz;
    int bx;
    int offset;
    int block_idx;

    offset = 0;
    for (bz = 0; bz < FG_BLOCK_Z_NUM; bz++) {
        for (bx = 0; bx < FG_BLOCK_X_NUM; bx++) {
            block_idx = (bz + 1) * BLOCK_X_NUM + (bx + 1);
            mMP_prepareBlockPreviewFromType(
                g_block_type_p[block_idx],
                &block_preview_textures[offset]);
            offset += MAP_PREVIEW_TEX_SIZE;
        }
    }

    /* Flush CPU cache to main memory so GX can read the textures */
    DCStoreRange(block_preview_textures, sizeof(block_preview_textures));
    DCStoreRange(preview_pal_colors, sizeof(preview_pal_colors));
}
#endif

extern void mMP_map_ovl_set_proc(Submenu* submenu) {
    Submenu_Overlay_c* overlay = submenu->overlay;

    overlay->menu_control.menu_move_func = &mMP_map_ovl_move;
    overlay->menu_control.menu_draw_func = &mMP_map_ovl_draw;
}

static void mMP_set_preview_init_data(mMP_Ovl_c* map_ovl) {
    mMP_LabelInfo_c* label_info = (mMP_LabelInfo_c*)map_ovl->label_info;
    int i;
    int bx;
    int bz;
    int block;
    int block_idx;
    u8 block_type;

    /* Default cursor to center of map */
    map_ovl->player_bx = 2;
    map_ovl->player_bz = 2;
    map_ovl->sel_bx = 2;
    map_ovl->sel_bz = 2;
    map_ovl->map_cursor_current_xpos = 2.0f * mMP_BLOCK_SIZE_F;
    map_ovl->map_cursor_current_zpos = -2.0f * mMP_BLOCK_SIZE_F;
    map_ovl->cursor_frame = 0;
    map_ovl->map_cursor_target_xpos = map_ovl->map_cursor_current_xpos;
    map_ovl->map_cursor_target_zpos = map_ovl->map_cursor_current_zpos;

    /* Clear labels — no residents/buildings to show in preview */
    for (i = 0; i < FG_BLOCK_TOTAL_NUM; i++, label_info++) {
        label_info->label_cnt = 0;
    }

    /* Populate map textures from block types using the existing lookup tables */
    block = 0;
    for (bz = 0; bz < FG_BLOCK_Z_NUM; bz++) {
        for (bx = 0; bx < FG_BLOCK_X_NUM; bx++) {
            block_idx = (bz + 1) * BLOCK_X_NUM + (bx + 1);
            block_type = g_block_type_p[block_idx];
            if (block_type >= mFM_BLOCK_TYPE_NUM) {
                block_type = mFM_BLOCK_TYPE_FLAT;
            }
            map_ovl->map_texture[block] = l_map_texture[block_type];
            map_ovl->map_pal[block] = l_map_pal[block_type];
            block++;
        }
    }

    map_ovl->land_name_str_len = mMl_strlen(Save_Get(land_info.name), LAND_NAME_SIZE, CHAR_SPACE);
}

static void mMP_map_ovl_init(Submenu* submenu) {
    Submenu_Overlay_c* overlay = submenu->overlay;
    mSM_MenuInfo_c* menu = &overlay->menu_info[mSM_OVL_MAP];

    overlay->menu_control.animation_flag = FALSE;
    (*submenu->overlay->move_chg_base_proc)(menu, mSM_MOVE_IN_TOP);

    sAdo_SysTrgStart(0x17C);
#if VERSION >= VER_DELUXE
    if (menu->data0 == 2) {
        mMP_set_preview_init_data(submenu->overlay->map_ovl);
    } else
#endif
    {
        mMP_set_init_data(submenu->overlay->map_ovl, menu);
    }
}

extern void mMP_map_ovl_construct(Submenu* submenu) {
    Submenu_Overlay_c* overlay = submenu->overlay;

    if (overlay->map_ovl == NULL) {
        overlay->map_ovl = (mMP_Ovl_c*)zelda_malloc(sizeof(mMP_Ovl_c));
        mem_clear((u8*)overlay->map_ovl, sizeof(mMP_Ovl_c), 0);
        overlay->map_ovl->combination_table = data_combi_table;
        mMP_map_draw_init();
    }

    mMP_map_ovl_init(submenu);
    mMP_map_ovl_set_proc(submenu);
}

extern void mMP_map_ovl_destruct(Submenu* submenu) {
    mMP_Ovl_c* map_ovl = submenu->overlay->map_ovl;

    if (map_ovl != NULL) {
        zelda_free(map_ovl);
    }

    submenu->overlay->map_ovl = NULL;
}
