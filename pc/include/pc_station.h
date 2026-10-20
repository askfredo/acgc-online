/* pc_station.h - Porter's online travel (see pc_station.c). */
#ifndef PC_STATION_H
#define PC_STATION_H

#include "pc_msg.h"

#define PC_STATION_MENU_MSG (PC_MSG_BASE + 0x100)          /* "going on a trip?" */
#define PC_STATION_TRAIN_MSG (PC_MSG_BASE + 0x101)         /* "where would you like to go?" */
#define PC_STATION_INVITE_MSG (PC_MSG_BASE + 0x102)
#define PC_STATION_INVITE_CODE_MSG (PC_MSG_BASE + 0x103)
#define PC_STATION_TICKET_MSG (PC_MSG_BASE + 0x104)        /* opens the keyboard */
#define PC_STATION_TICKET_RESULT_MSG (PC_MSG_BASE + 0x105)
#define PC_STATION_NEVER_MIND_MSG (PC_MSG_BASE + 0x106)
#define PC_STATION_WORLD_MSG (PC_MSG_BASE + 0x107)         /* opens the world map */
#define PC_STATION_WORLD_PICK_MSG (PC_MSG_BASE + 0x108)    /* "<town>! What would you like to do?" */
#define PC_STATION_MOVE_RESULT_MSG (PC_MSG_BASE + 0x109)
#define PC_STATION_VISIT_SOON_MSG (PC_MSG_BASE + 0x10A)
#define PC_STATION_MOVE_CHECK_MSG (PC_MSG_BASE + 0x10B)    /* moves in when it shows */
#define PC_STATION_WORLD_CANCEL_MSG (PC_MSG_BASE + 0x10C)
#define PC_STATION_WORLD_EMPTY_MSG (PC_MSG_BASE + 0x10D)
#define PC_STATION_PUBLIC_YES_MSG (PC_MSG_BASE + 0x10E)
#define PC_STATION_PUBLIC_NO_MSG (PC_MSG_BASE + 0x10F)
#define PC_STATION_TOWN_MSG(i) (PC_MSG_BASE + 0x110 + (i))

/* Prepares Porter's lines; returns the first message, or 0 to keep his original talk. */
int pc_station_open(void);
/* "Invite friends" line is showing: put the town online and fill in its code. */
void pc_station_invite(const char* town_name);
/* Ticket typed on the keyboard: join that town; returns the message to show. */
int pc_station_ticket(const char* code);
/* World map: fetch the published towns (returns how many), show the map, and once it
 * closes the message to continue with (0 while it's still open). */
int pc_station_world_load(void);
void pc_station_world_open(void);
int pc_station_world_done(void);
/* "Move in" on a town picked on the map (its check message is showing). */
void pc_station_move_in(void);
/* After the invite: list the town on the world map, or keep it private. */
void pc_station_set_public(int is_public);

/* A town was picked on the train menu. */
void pc_station_board(int town_idx);
/* Porter's talk ended: board the train if a destination was set. */
void pc_station_talk_done(void);
void pc_station_tick(void);
int pc_station_town_count(void);

#endif /* PC_STATION_H */
