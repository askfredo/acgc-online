/* pc_menu.h - start menu shown before the game boots: online towns or offline play. */
#ifndef PC_MENU_H
#define PC_MENU_H

#ifdef __cplusplus
extern "C" {
#endif

enum {
    PC_MENU_QUIT,
    PC_MENU_OFFLINE,
    PC_MENU_CLOUD_TOWN, /* a town was picked: pc_cloud_select_town() was called */
};

/* Runs in the game window until the player picks something. */
int pc_menu_run(void);

#ifdef __cplusplus
}
#endif

#endif /* PC_MENU_H */
