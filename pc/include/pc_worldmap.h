/* pc_worldmap.h - the world map Porter shows: published towns drawn over the game.
 *
 * While open it takes the controller (the game sees nothing) and draws itself over the
 * game's frame. Porter's talk waits for a result. */
#ifndef PC_WORLDMAP_H
#define PC_WORLDMAP_H

#include "pc_cloud.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PC_WORLDMAP_OPEN (-2)
#define PC_WORLDMAP_CANCEL (-1)

void pc_worldmap_open(const PcWorldTown* towns, int count);
int pc_worldmap_is_open(void);
/* PC_WORLDMAP_OPEN while showing, then PC_WORLDMAP_CANCEL or the chosen town's index. */
int pc_worldmap_result(void);

/* From the pad layer: the map's input (buttons are PAD_BUTTON_* bits). */
void pc_worldmap_input(unsigned buttons, int stick_x, int stick_y);
/* From the buffer swap: draw over the game's frame. */
void pc_worldmap_draw(void);

#ifdef __cplusplus
}
#endif

#endif /* PC_WORLDMAP_H */
