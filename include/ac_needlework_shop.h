#ifndef AC_NEEDLEWORK_SHOP_H
#define AC_NEEDLEWORK_SHOP_H

#include "types.h"
#include "ac_structure.h"

#ifdef __cplusplus
extern "C" {
#endif

extern ACTOR_PROFILE Needlework_Shop_Profile;
#if VERSION >= VER_DELUXE
extern int aNW_get_open_time(void);
extern int aNW_get_close_time(void);
#endif

#ifdef __cplusplus
}
#endif

#endif

