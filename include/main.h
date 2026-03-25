#ifndef MAIN_H
#define MAIN_H

#include "types.h"
#include "dolphin/os/OSThread.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

#define SCREEN_WIDTH_F ((f32)SCREEN_WIDTH)
#define SCREEN_HEIGHT_F ((f32)SCREEN_HEIGHT)

extern int ScreenWidth;
extern int ScreenHeight;

#if VERSION >= VER_DELUXE
/* 16:9 widescreen support.
 * The GC EFB remains 640x480; widescreen works by adjusting the projection
 * matrix aspect ratio to 16:9 (anamorphic).  Dolphin's "Force 16:9" or
 * "Stretch to Window" then displays the result correctly.
 *
 * WIDESCREEN_ASPECT_SCALE is the horizontal multiplier applied to the
 * projection: 16/9 / (4/3) = 4/3 = 1.333...  This widens the view frustum
 * so that more of the world is visible horizontally. */
#define WIDESCREEN_ASPECT_SCALE (16.0f / 12.0f)  /* (16/9) / (4/3) = 4/3 */
#define SCREEN_WIDTH_WIDE 426  /* 240 * 16/9 ~ 426.67 -> 426 */
#define SCREEN_WIDTH_WIDE_F ((f32)SCREEN_WIDTH_WIDE)

extern u8 widescreen_enabled;

/* Effective screen width accounting for widescreen mode */
#define EFFECTIVE_SCREEN_WIDTH (widescreen_enabled ? SCREEN_WIDTH_WIDE : SCREEN_WIDTH)
#define EFFECTIVE_SCREEN_WIDTH_F (widescreen_enabled ? SCREEN_WIDTH_WIDE_F : SCREEN_WIDTH_F)
#endif /* VER_DELUXE */

extern OSThread graphThread;
extern u8 SegmentBaseAddress[0x40];

void main();
extern void mainproc(void* val);

#ifdef __cplusplus
}
#endif

#endif
