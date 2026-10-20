/* pc_station.c - Porter (the train station) takes you to towns online.
 *
 * "Are you planning on going on a trip?" offers: Take the train (to the other towns you
 * live in, or to a friend's town with their ticket, where you move in as a new
 * resident), Invite friends (puts this town online and gives its ticket code) or
 * No thanks. The station actor drives the talk (ac_npc_station_master_online.c_inc);
 * this file builds Porter's lines and does the server work.
 *
 * Travelling saves the game, then restarts it in the chosen town (a town has to be
 * loaded before the game boots). */
#include "pc_cloud.h"
#include "pc_msg.h"
#include "pc_station.h"
#include "pc_worldmap.h"
#include "pc_net.h"

#include <SDL.h>
#include <stdio.h>
#include <string.h>

#define CHOICE_TRAIN (PC_CHOICE_STR_BASE + 0x20)
#define CHOICE_INVITE (PC_CHOICE_STR_BASE + 0x21)
#define CHOICE_NO_THANKS (PC_CHOICE_STR_BASE + 0x22)
#define CHOICE_TICKET (PC_CHOICE_STR_BASE + 0x23)
#define CHOICE_NEVER_MIND (PC_CHOICE_STR_BASE + 0x24)
#define CHOICE_WORLD_MAP (PC_CHOICE_STR_BASE + 0x25)
#define CHOICE_MOVE_IN (PC_CHOICE_STR_BASE + 0x26)
#define CHOICE_VISIT (PC_CHOICE_STR_BASE + 0x27)
#define CHOICE_TOWN(i) (PC_CHOICE_STR_BASE + 0x28 + (i))
#define CHOICE_GO_THERE (PC_CHOICE_STR_BASE + 0x30)
#define CHOICE_MAP_YES (PC_CHOICE_STR_BASE + 0x31)
#define CHOICE_MAP_NO (PC_CHOICE_STR_BASE + 0x32)

#define PORTER_NO_THANKS_MSG 0x0944 /* Porter's own "that's a shame" */
#define STATION_MAX_TOWNS 4         /* 6 choice lines: towns + ticket + never mind */

extern void pc_relaunch_with(const char* arg, const char* value);
extern int mMsg_PC_IsWindowIdle(void);
extern int mCD_SaveHome_bg(int param_1, int* chan);

static PcCloudTown s_towns[STATION_MAX_TOWNS];
static int s_town_count = 0;
static char s_depart_town[32] = ""; /* set when the player boards: relaunch here */
static int s_depart_ready = 0;
static PcWorldTown s_world[PC_WORLD_MAX_TOWNS];
static int s_world_count = 0;
static int s_world_pick = -1;

static void pc_station_hex_id(char* out, size_t size, int id) {
    size_t n = strlen(out);
    snprintf(out + n, size - n, "%s%02X %02X", n ? " " : "", id >> 8, id & 0xFF);
}

static void pc_station_next(char* out, size_t size, int slot, int msg) {
    size_t n = strlen(out);
    snprintf(out + n, size - n, "<<SETNEXTMSG%d [%02X %02X]>>", slot, msg >> 8, msg & 0xFF);
}

static const char* pc_station_selstr(int n) {
    static const char* const names[] = { NULL, NULL, "SETSELSTR2", "SETSELSTR3", "SETSELSTR4", "SETSELSTR5",
                                         "SETSELSTR6" };
    return names[n];
}

int pc_station_open(void) {
    PcCloudTown all[PC_CLOUD_MAX_TOWNS];
    char err[160];
    char text[1536];
    char ids[96] = "";
    char nexts[320] = "";
    int n;
    int i;
    int choices = 0;

    s_town_count = 0;
    s_depart_town[0] = '\0'; /* a new talk: no destination yet */
    s_depart_ready = 0;
    if (!pc_cloud_ensure_account(err, sizeof(err))) {
        printf("[Station] offline (%s): Porter keeps his own talk\n", err);
        return 0;
    }

    /* Other towns this player lives in (not the one being played). */
    n = pc_cloud_list_towns(all, PC_CLOUD_MAX_TOWNS, err, sizeof(err));
    for (i = 0; i < n && s_town_count < STATION_MAX_TOWNS; i++) {
        if (!g_pc_cloud_mode || strcmp(all[i].id, g_pc_cloud_town) != 0) {
            s_towns[s_town_count++] = all[i];
        }
    }

    /* "Are you planning on going on a trip?" */
    pc_msg_set_choice(CHOICE_TRAIN, "Take the train");
    pc_msg_set_choice(CHOICE_INVITE, "Invite friends");
    pc_msg_set_choice(CHOICE_NO_THANKS, "No thanks");
    pc_msg_set_choice(CHOICE_WORLD_MAP, "World map");
    pc_msg_set(PC_STATION_MENU_MSG,
               "Are you planning on going\n"
               "on a trip,<<PAUSE [04]>> <<STR_PLAYERNAME>>?"
               "<<SETSELSTR4 [07 20 07 25 07 21 07 22]>><<SELNOB>><<BTN>><<OPENCHOICE>>"
               "<<SETNEXTMSG0 [71 01]>><<SETNEXTMSG1 [71 07]>><<SETNEXTMSG2 [71 02]>><<SETNEXTMSG3 [09 44]>>"
               "<<FORCENEXT>>\n"
               "<<MSGCONTINUE>>");
    pc_msg_set(PC_STATION_WORLD_MSG,
               "The world map,<<PAUSE [06]>> eek?<<PAUSE [08]>> Every town\n"
               "on it welcomes visitors.<<PAUSE [08]>>\n"
               "Here,<<PAUSE [04]>> take a look!<<PAUSE [10]>>\n"
               "<<DEMONPC0 [09 00 01]>>\n"
               "<<FORCENEXT>>\n"
               "<<MSGCONTINUE>>");
    pc_msg_set(PC_STATION_WORLD_CANCEL_MSG,
               "Nothing caught your eye,<<PAUSE [06]>> eek?<<PAUSE [08]>>\n"
               "New towns show up all the\n"
               "time.<<PAUSE [06]>> Come back anytime!\n"
               "<<MSGEND>>");
    pc_msg_set(PC_STATION_WORLD_EMPTY_MSG,
               "Eek!<<PAUSE [08]>> There aren't any towns\n"
               "on the map yet.<<BTN>>\n"
               "<<MSGCLEAR>>Why not be the first?<<PAUSE [08]>>\n"
               "Choose <<COLORCHARS [E1 1E D7 0E]>>Invite friends and put\n"
               "your town on the map!<<PAUSE [06]>> Eek eek!\n"
               "<<MSGEND>>");
    pc_msg_set(PC_STATION_VISIT_SOON_MSG,
               "Just a visit?<<PAUSE [08]>> Eek...<<PAUSE [08]>> The visitor\n"
               "trains aren't running yet.<<BTN>>\n"
               "<<MSGCLEAR>>They'll start very soon,<<PAUSE [06]>>\n"
               "I promise!<<PAUSE [06]>> Eek eek!\n"
               "<<MSGEND>>");
    pc_msg_set(PC_STATION_MOVE_CHECK_MSG,
               "Moving in,<<PAUSE [06]>> eek?<<PAUSE [08]>> Let me see\n"
               "if there's a house free...<<BTN>>\n"
               "<<SETFORCEMSG [71 09]>>\n"
               "<<FORCENEXT>>\n"
               "<<MSGCONTINUE>>");
    pc_msg_set(PC_STATION_PUBLIC_YES_MSG,
               "Wonderful!<<PAUSE [08]>> Your town is on\n"
               "the world map now.<<PAUSE [08]>> Expect\n"
               "visitors,<<PAUSE [06]>> eek eek!\n"
               "<<MSGEND>>");
    pc_msg_set(PC_STATION_PUBLIC_NO_MSG,
               "Just your friends,<<PAUSE [06]>> then.<<PAUSE [08]>>\n"
               "Your town stays off the map.<<PAUSE [06]>>\n"
               "Eek eek!\n"
               "<<MSGEND>>");
    pc_msg_set_choice(CHOICE_MAP_YES, "Yes, please");
    pc_msg_set_choice(CHOICE_MAP_NO, "No, thanks");
    pc_msg_set_choice(CHOICE_MOVE_IN, "Move in");
    pc_msg_set_choice(CHOICE_VISIT, "Visit");
    pc_msg_set_choice(CHOICE_GO_THERE, "Go there");

    /* "Where would you like to go?" */
    for (i = 0; i < s_town_count; i++) {
        char name[48];
        char line[512];
        const char* me = pc_cloud_get_user();

        pc_msg_sanitize(s_towns[i].name, name, sizeof(name));
        pc_msg_set_choice(CHOICE_TOWN(i), name);
        if (s_towns[i].host[0] != '\0' && strcmp(s_towns[i].host, me) != 0) {
            /* They're playing there now: the train joins their game. */
            snprintf(line, sizeof(line),
                     "To %s,<<PAUSE [06]>> eek?<<PAUSE [08]>> Someone's\n"
                     "playing there right now,<<PAUSE [06]>> so\n"
                     "you'll play together!<<BTN>>\n"
                     "<<MSGCLEAR>>I'll save your game before\n"
                     "you board.<<PAUSE [08]>> Eek eek!\n"
                     "<<MSGEND>>",
                     name);
        } else {
            snprintf(line, sizeof(line),
                     "To %s,<<PAUSE [06]>> eek?<<PAUSE [08]>> Wonderful!<<BTN>>\n"
                     "<<MSGCLEAR>>I'll save your game before\n"
                     "you board.<<PAUSE [08]>> Have a nice trip,\n"
                     "<<STR_PLAYERNAME>>!<<PAUSE [06]>> Eek eek!\n"
                     "<<MSGEND>>",
                     name);
        }
        pc_msg_set(PC_STATION_TOWN_MSG(i), line);
        pc_station_hex_id(ids, sizeof(ids), CHOICE_TOWN(i));
        pc_station_next(nexts, sizeof(nexts), choices++, PC_STATION_TOWN_MSG(i));
    }
    pc_msg_set_choice(CHOICE_TICKET, "I have a ticket");
    pc_msg_set_choice(CHOICE_NEVER_MIND, "Never mind");
    pc_station_hex_id(ids, sizeof(ids), CHOICE_TICKET);
    pc_station_next(nexts, sizeof(nexts), choices++, PC_STATION_TICKET_MSG);
    pc_station_hex_id(ids, sizeof(ids), CHOICE_NEVER_MIND);
    pc_station_next(nexts, sizeof(nexts), choices++, PC_STATION_NEVER_MIND_MSG);
    snprintf(text, sizeof(text),
             "Eek!<<PAUSE [06]>> Where would you like\n"
             "to go today?"
             "<<%s [%s]>><<SELNOB>><<BTN>><<OPENCHOICE>>%s<<FORCENEXT>>\n"
             "<<MSGCONTINUE>>",
             pc_station_selstr(choices), ids, nexts);
    pc_msg_set(PC_STATION_TRAIN_MSG, text);

    pc_msg_set(PC_STATION_NEVER_MIND_MSG,
               "All right,<<PAUSE [06]>> eek.<<PAUSE [08]>> The train will\n"
               "be here whenever you're ready!\n"
               "<<MSGEND>>");

    /* The ticket: the station actor opens the game's keyboard after this line. */
    pc_msg_set(PC_STATION_TICKET_MSG,
               "A ticket from a friend,<<PAUSE [06]>> eek?<<PAUSE [08]>>\n"
               "Let me see it!<<PAUSE [10]>>\n"
               "<<DEMONPC0 [09 00 01]>>\n"
               "<<FORCENEXT>>\n"
               "<<MSGCONTINUE>>");

    /* Invite: the code is filled in when this line shows (pc_station_invite). */
    pc_msg_set(PC_STATION_INVITE_MSG,
               "Invite your friends,<<PAUSE [06]>> eek?<<PAUSE [08]>>\n"
               "What a lovely idea!<<PAUSE [08]>> Let me\n"
               "print you a ticket.<<BTN>>\n"
               "<<SETFORCEMSG [71 03]>>\n"
               "<<FORCENEXT>>\n"
               "<<MSGCONTINUE>>");
    pc_msg_set(PC_STATION_INVITE_CODE_MSG,
               "Hmm,<<PAUSE [06]>> the ticket printer is\n"
               "jammed,<<PAUSE [06]>> eek.<<PAUSE [08]>> Try again later!\n"
               "<<MSGEND>>");

    printf("[Station] Porter knows %d other town(s)\n", s_town_count);
    return PC_STATION_MENU_MSG;
}

void pc_station_invite(const char* town_name) {
    char code[16];
    char err[160];
    char text[640];

    if (g_pc_net_mode == PC_NET_MODE_JOIN) {
        /* A guest in someone else's game: the town is theirs to share. */
        pc_msg_set(PC_STATION_INVITE_CODE_MSG,
                   "Eek!<<PAUSE [08]>> You're playing in someone\n"
                   "else's game right now.<<BTN>>\n"
                   "<<MSGCLEAR>>Ask them for the ticket,<<PAUSE [06]>>\n"
                   "eek eek!\n"
                   "<<MSGEND>>");
        return;
    }

    if (!pc_cloud_go_online(town_name, code, sizeof(code), err, sizeof(err))) {
        printf("[Station] invite failed: %s\n", err);
        return; /* the "printer is jammed" line stays */
    }
    SDL_SetClipboardText(code);
    snprintf(text, sizeof(text),
             "Here you go,<<PAUSE [06]>> eek!<<BTN>>\n"
             "<<MSGCLEAR>>Your friends just need to\n"
             "show me this ticket at\n"
             "their own station:<<BTN>>\n"
             "<<MSGCLEAR>><<PAUSE [08]>>%s<<PAUSE [10]>><<BTN>>\n"
             "<<MSGCLEAR>>It's written on your\n"
             "clipboard,<<PAUSE [06]>> too.<<PAUSE [08]>> Your town is\n"
             "online now.<<BTN>>\n"
             "<<MSGCLEAR>>Would you like it on the\n"
             "world map too,<<PAUSE [06]>> so anyone\n"
             "can find it?"
             "<<SETSELSTR2 [07 31 07 32]>><<SELNOB>><<BTN>><<OPENCHOICE>>"
             "<<SETNEXTMSG0 [71 0E]>><<SETNEXTMSG1 [71 0F]>><<FORCENEXT>>\n"
             "<<MSGCONTINUE>>",
             code);
    pc_msg_set(PC_STATION_INVITE_CODE_MSG, text);
}

int pc_station_ticket(const char* code) {
    PcCloudTown town;
    char err[160];
    char name[48];
    char text[640];

    s_depart_town[0] = '\0';
    if (code[0] == '\0' || !pc_cloud_join_town(code, &town, err, sizeof(err))) {
        pc_msg_set(PC_STATION_TICKET_RESULT_MSG,
                   "Eek eek...<<PAUSE [08]>> I'm sorry,<<PAUSE [06]>> but\n"
                   "that ticket doesn't work.<<BTN>>\n"
                   "<<MSGCLEAR>>Could you ask your friend\n"
                   "for it again?<<PAUSE [06]>> Eek!\n"
                   "<<MSGEND>>");
        return PC_STATION_TICKET_RESULT_MSG;
    }

    pc_msg_sanitize(town.name, name, sizeof(name));
    snprintf(text, sizeof(text),
             "Eek!<<PAUSE [08]>> A ticket to %s!<<BTN>>\n"
             "<<MSGCLEAR>>Your friends will be glad to\n"
             "have a new neighbor.<<PAUSE [08]>>\n"
             "I'll save your game before\n"
             "you board.<<PAUSE [08]>> Eek eek!\n"
             "<<MSGEND>>",
             name);
    pc_msg_set(PC_STATION_TICKET_RESULT_MSG, text);
    snprintf(s_depart_town, sizeof(s_depart_town), "%s", town.id);
    return PC_STATION_TICKET_RESULT_MSG;
}

void pc_station_board(int town_idx) {
    if (town_idx >= 0 && town_idx < s_town_count) {
        snprintf(s_depart_town, sizeof(s_depart_town), "%s", s_towns[town_idx].id);
    }
}

int pc_station_world_load(void) {
    char err[160];
    int n = pc_cloud_world(s_world, PC_WORLD_MAX_TOWNS, err, sizeof(err));

    if (n < 0) {
        printf("[Station] world map: %s\n", err);
        n = 0;
    }
    s_world_count = n;
    s_world_pick = -1;
    return n;
}

void pc_station_world_open(void) {
    pc_worldmap_open(s_world, s_world_count);
}

int pc_station_world_done(void) {
    char name[48];
    char text[768];
    const PcWorldTown* t;
    int pick;

    if (s_world_count == 0) {
        return PC_STATION_WORLD_EMPTY_MSG;
    }
    pick = pc_worldmap_result();
    if (pick == PC_WORLDMAP_OPEN) {
        return 0;
    }
    if (pick < 0) {
        return PC_STATION_WORLD_CANCEL_MSG;
    }

    s_world_pick = pick;
    t = &s_world[pick];
    pc_msg_sanitize(t->name, name, sizeof(name));
    if (g_pc_cloud_mode && strcmp(t->id, g_pc_cloud_town) == 0) {
        snprintf(text, sizeof(text),
                 "%s?<<PAUSE [08]>> Eek eek!<<PAUSE [08]>> That's\n"
                 "where we are right now,\n"
                 "<<STR_PLAYERNAME>>!\n"
                 "<<MSGEND>>",
                 name);
        s_world_pick = -1;
    } else if (t->mine) {
        /* already a resident: just a trip there */
        snprintf(text, sizeof(text),
                 "%s,<<PAUSE [06]>> eek!<<PAUSE [08]>> You live\n"
                 "there,<<PAUSE [06]>> don't you?<<PAUSE [08]>> Shall we go?"
                 "<<SETSELSTR2 [07 30 07 24]>><<SELNOB>><<BTN>><<OPENCHOICE>>"
                 "<<SETNEXTMSG0 [71 0B]>><<SETNEXTMSG1 [71 06]>><<FORCENEXT>>\n"
                 "<<MSGCONTINUE>>",
                 name);
    } else {
        snprintf(text, sizeof(text),
                 "%s,<<PAUSE [06]>> eek!<<PAUSE [08]>> What a\n"
                 "lovely place.<<PAUSE [08]>> What would you\n"
                 "like to do?"
                 "<<SETSELSTR3 [07 26 07 27 07 24]>><<SELNOB>><<BTN>><<OPENCHOICE>>"
                 "<<SETNEXTMSG0 [71 0B]>><<SETNEXTMSG1 [71 0A]>><<SETNEXTMSG2 [71 06]>><<FORCENEXT>>\n"
                 "<<MSGCONTINUE>>",
                 name);
    }
    pc_msg_set(PC_STATION_WORLD_PICK_MSG, text);
    return PC_STATION_WORLD_PICK_MSG;
}

void pc_station_move_in(void) {
    PcCloudTown town;
    char err[160];
    char name[48];
    char text[640];
    const PcWorldTown* t;

    s_depart_town[0] = '\0';
    if (s_world_pick < 0) {
        return;
    }
    t = &s_world[s_world_pick];
    pc_msg_sanitize(t->name, name, sizeof(name));
    if (!pc_cloud_move_in(t->id, &town, err, sizeof(err))) {
        printf("[Station] move in: %s\n", err);
        snprintf(text, sizeof(text),
                 "Eek eek...<<PAUSE [08]>> I'm afraid every house\n"
                 "in %s is taken.<<BTN>>\n"
                 "<<MSGCLEAR>>But a visit is always a nice\n"
                 "idea!<<PAUSE [06]>> Eek!\n"
                 "<<MSGEND>>",
                 name);
    } else {
        snprintf(text, sizeof(text),
                 "Eek!<<PAUSE [08]>> %s it is!<<BTN>>\n"
                 "<<MSGCLEAR>>I'll save your game before\n"
                 "you board.<<PAUSE [08]>> Have a nice trip,\n"
                 "<<STR_PLAYERNAME>>!<<PAUSE [06]>> Eek eek!\n"
                 "<<MSGEND>>",
                 name);
        snprintf(s_depart_town, sizeof(s_depart_town), "%s", t->id);
    }
    pc_msg_set(PC_STATION_MOVE_RESULT_MSG, text);
}

void pc_station_set_public(int is_public) {
    char err[160];

    if (!pc_cloud_set_public(is_public, err, sizeof(err))) {
        printf("[Station] could not %s the map: %s\n", is_public ? "join" : "leave", err);
    }
}

void pc_station_talk_done(void) {
    if (s_depart_town[0] != '\0') {
        s_depart_ready = 1;
    }
}

/* Every frame: once Porter is done talking, save and take the train. */
void pc_station_tick(void) {
    int chan;

    if (!s_depart_ready || !mMsg_PC_IsWindowIdle()) {
        return;
    }
    s_depart_ready = 0;
    printf("[Station] saving and taking the train to %s\n", s_depart_town);
    mCD_SaveHome_bg(0, &chan);
    pc_relaunch_with("--town", s_depart_town);
}

int pc_station_town_count(void) {
    return s_town_count;
}
