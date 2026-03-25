#ifndef LIBULTRA_H
#define LIBULTRA_H

#include "types.h"
#include "dolphin/os/OSTime.h"
#include "dolphin/os/OSCache.h"
#include "libultra/gu.h"
#include "libultra/osMesg.h"
#include "libultra/shutdown.h"
#include "libultra/os_timer.h"
#include "libultra/os_thread.h"
#include "libultra/os_pi.h"
#include "libultra/initialize.h"
#include "libc64/math64.h" /* TODO: sins and coss belong in libultra */

#define N64_SCREEN_HEIGHT 240
#define N64_SCREEN_WIDTH 320

#ifdef __cplusplus
extern "C" {
#endif

typedef u64 Z_OSTime;

/* On PC/Linux, bcmp/bcopy/bzero are already declared in <strings.h> with
   const-qualified params.  Avoid conflicting redeclarations. */
#ifndef TARGET_PC
int bcmp(void* v1, void* v2, u32 size);
void bcopy(void* src, void* dst, size_t n);
void bzero(void* ptr, size_t size);
#else
#include <strings.h>
  #ifdef _WIN32
    /* MinGW strings.h doesn't declare bzero/bcopy — implemented in pc_misc.c */
    #ifdef __cplusplus
    extern "C" {
    #endif
    void bzero(void* s, unsigned int n);
    void bcopy(const void* src, void* dst, unsigned int n);
    #ifdef __cplusplus
    }
    #endif
  #endif
#endif
void osSyncPrintf(const char* fmt, ...);
void osWritebackDCache(void* vaddr, u32 nbytes);
u32 osGetCount(void);
OSTime osGetTime(void);

extern s32 osAppNMIBuffer[16];
extern int osShutdown;
extern u8 __osResetSwitchPressed;

#ifdef __cplusplus
}
#endif

#endif
