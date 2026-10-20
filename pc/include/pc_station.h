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
#define PC_STATION_TOWN_MSG(i) (PC_MSG_BASE + 0x110 + (i))

/* Prepares Porter's lines; returns the first message, or 0 to keep his original talk. */
int pc_station_open(void);
/* "Invite friends" line is showing: put the town online and fill in its code. */
void pc_station_invite(const char* town_name);
/* Ticket typed on the keyboard: join that town; returns the message to show. */
int pc_station_ticket(const char* code);
/* A town was picked on the train menu. */
void pc_station_board(int town_idx);
/* Porter's talk ended: board the train if a destination was set. */
void pc_station_talk_done(void);
void pc_station_tick(void);
int pc_station_town_count(void);

#endif /* PC_STATION_H */
