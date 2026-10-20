/* pc_online_intro.c - the game's own characters lead you to online play.
 *
 * K.K. Slider's first-boot talk ends with "you'll enjoy it more if you get some friends
 * to come here". Here he goes on to ask whether you're heading to a town online with
 * your friends or starting out on your own.
 *
 * Online: on the train, when Rover asks "Where's your new place?", the choices are your
 * towns on the server plus "A new town". Picking a town restarts the game in it (it has
 * to be loaded before boot); a new town carries on with the usual intro and goes online
 * at its first save. On your own: the original intro, offline. */
#include "pc_cloud.h"
#include "pc_msg.h"

#include <stdio.h>
#include <string.h>

/* K.K.'s last first-boot message (player select, no save yet). */
#define KK_FRIENDS_MSG 0x09C9
#define KK_ONLINE_MSG (PC_MSG_BASE + 0x01)
#define KK_ON_MY_OWN_MSG (PC_MSG_BASE + 0x02)
/* Rover on the train: "Where's your new place?" and the message that follows. */
#define ROVER_WHERE_MSG 0x2ACC
#define ROVER_TOWN_MSG(i) (PC_MSG_BASE + 0x10 + (i))
#define ROVER_NEW_TOWN_MSG (PC_MSG_BASE + 0x20)

#define CHOICE_ONLINE (PC_CHOICE_STR_BASE + 0x00)
#define CHOICE_ON_MY_OWN (PC_CHOICE_STR_BASE + 0x01)
#define CHOICE_NEW_TOWN (PC_CHOICE_STR_BASE + 0x02)
#define CHOICE_TOWN(i) (PC_CHOICE_STR_BASE + 0x10 + (i))

#define ROVER_MAX_TOWNS 5 /* the game's choice window holds 6 lines */

extern void pc_relaunch_with(const char* arg, const char* value);
extern int mMsg_PC_IsWindowIdle(void);

static PcCloudTown s_towns[ROVER_MAX_TOWNS];
static int s_town_count = 0;
static int s_chosen_town = -1;

void pc_online_intro_init(void) {
    int ok = 1;

    if (g_pc_cloud_mode) {
        return; /* already playing a town from the server: nothing to ask */
    }

    /* Same opening as the original message, then the question. */
    ok &= pc_msg_override(KK_FRIENDS_MSG,
                          "<<DEMONPC0 [00 00 28]>><<PAUSE [05]>>Oh!<<PAUSE [04]>> I almost forgot!<<BTN>>"
                          "<<DEMONPC0 [00 00 FD]>>\n"
                          "<<MSGCLEAR>>You'll enjoy your time in the\n"
                          "world of Animal Crossing\n"
                          "more<<PAUSE [08]>> if you get some friends\n"
                          "to come here, too.<<BTN>>\n"
                          "<<MSGCLEAR>>And dig this:<<PAUSE [06]>> your friends\n"
                          "can live in the same town,<<PAUSE [06]>>\n"
                          "and you can all play together.<<BTN>>\n"
                          "<<MSGCLEAR>>So,<<PAUSE [06]>> are you heading to a town\n"
                          "online<<PAUSE [06]>> with your friends,<<PAUSE [06]>> or\n"
                          "starting out on your own?"
                          "<<SETSELSTR2 [07 00 07 01]>><<BTN>><<OPENCHOICE>>"
                          "<<SETNEXTMSG0 [70 01]>><<SETNEXTMSG1 [70 02]>><<FORCENEXT>>\n"
                          "<<MSGCONTINUE>>");
    ok &= pc_msg_set(KK_ONLINE_MSG,
                     "Far out!<<PAUSE [08]>> Your friends are\n"
                     "waiting for you.<<BTN>>\n"
                     "<<MSGCLEAR>>Later.\n"
                     "<<MSGEND>>");
    ok &= pc_msg_set(KK_ON_MY_OWN_MSG,
                     "Cool.<<PAUSE [06]>> Your own town,<<PAUSE [06]>>\n"
                     "your own rules.<<BTN>>\n"
                     "<<MSGCLEAR>>Later.\n"
                     "<<MSGEND>>");
    ok &= pc_msg_set_choice(CHOICE_ONLINE, "Online!");
    ok &= pc_msg_set_choice(CHOICE_ON_MY_OWN, "On my own");
    if (!ok) {
        fprintf(stderr, "[Online] could not set up K.K.'s dialogue\n");
    }
}

/* Rover's "Where's your new place?" with your towns as the choices. */
static void pc_online_intro_setup_rover(void) {
    static const char* const selstr[] = { NULL, NULL, "SETSELSTR2", "SETSELSTR3", "SETSELSTR4", "SETSELSTR5",
                                          "SETSELSTR6" };
    PcCloudTown all[PC_CLOUD_MAX_TOWNS];
    char err[160];
    char text[1536];
    char ids[64] = "";
    char nexts[256] = "";
    int n;
    int i;
    int len;

    if (!pc_cloud_ensure_account(err, sizeof(err))) {
        printf("[Online] no town server (%s): the intro carries on offline\n", err);
        return;
    }
    n = pc_cloud_list_towns(all, PC_CLOUD_MAX_TOWNS, err, sizeof(err));
    if (n < 0) {
        printf("[Online] could not list towns (%s): the intro carries on offline\n", err);
        return;
    }
    s_town_count = n > ROVER_MAX_TOWNS ? ROVER_MAX_TOWNS : n;
    memcpy(s_towns, all, s_town_count * sizeof(PcCloudTown));

    /* No towns yet: Rover's usual questions, and the town goes online at its first save. */
    if (s_town_count == 0) {
        pc_cloud_defer_new_town();
        return;
    }

    for (i = 0; i < s_town_count; i++) {
        char name[48];

        pc_msg_sanitize(s_towns[i].name, name, sizeof(name));
        pc_msg_set_choice(CHOICE_TOWN(i), name);
        snprintf(text, sizeof(text),
                 "%s?<<PAUSE [08]>> Oh, I know that place!<<PAUSE [08]>>\n"
                 "Your friends must be waiting\n"
                 "for you there.<<BTN>>\n"
                 "<<MSGCLEAR>>Next stop,<<PAUSE [06]>> %s!<<PAUSE [0A]>>\n"
                 "<<DEMONPC0 [00 00 03]>>Mya ha ha ha ha howr!\n"
                 "<<MSGEND>>",
                 name, name);
        pc_msg_set(ROVER_TOWN_MSG(i), text);
        len = (int)strlen(ids);
        snprintf(ids + len, sizeof(ids) - len, "%s%02X %02X", i ? " " : "", CHOICE_TOWN(i) >> 8, CHOICE_TOWN(i) & 0xFF);
        len = (int)strlen(nexts);
        snprintf(nexts + len, sizeof(nexts) - len, "<<SETNEXTMSG%d [%02X %02X]>>", i, ROVER_TOWN_MSG(i) >> 8,
                 ROVER_TOWN_MSG(i) & 0xFF);
    }
    pc_msg_set_choice(CHOICE_NEW_TOWN, "A new town");
    len = (int)strlen(ids);
    snprintf(ids + len, sizeof(ids) - len, " %02X %02X", CHOICE_NEW_TOWN >> 8, CHOICE_NEW_TOWN & 0xFF);
    len = (int)strlen(nexts);
    snprintf(nexts + len, sizeof(nexts) - len, "<<SETNEXTMSG%d [%02X %02X]>>", s_town_count,
             ROVER_NEW_TOWN_MSG >> 8, ROVER_NEW_TOWN_MSG & 0xFF);

    snprintf(text, sizeof(text),
             "<<DEMONPC0 [00 00 FE]>>Hrmm...<<PAUSE [08]>> Moving, huh?<<PAUSE [0C]>>\n"
             "I hate moving. <<PAUSE [08]>>Pack boxes,<<PAUSE [08]>>\n"
             "unpack boxes.<<PAUSE [08]>> It never ends.<<PAUSE [0A]>>\n"
             "Say!<<PAUSE [08]>> Where's your new place?"
             "<<%s [%s]>><<SELNOB>><<BTN>><<OPENCHOICE>>%s<<FORCENEXT>>\n"
             "<<MSGCONTINUE>>",
             selstr[s_town_count + 1], ids, nexts);
    pc_msg_override(ROVER_WHERE_MSG, text);

    /* A new town: Rover's own "let me call my buddy" ending, minus "you don't know yet?". */
    pc_msg_set(ROVER_NEW_TOWN_MSG,
               "<<DEMONPC0 [00 00 19]>><<PAUSE [08]>>A brand-new town?!?<<PAUSE [06]>>\n"
               "Groovy!<<PAUSE [08]>> Well, we'd better\n"
               "find you a place to live!!!<<BTN>>\n"
               "<<DEMONPC0 [00 00 FE]>>\n"
               "<<MSGCLEAR>><<PAUSE [06]>>.<<PAUSE [04]>>.<<PAUSE [04]>>.<<PAUSE [04]>><<DEMONPC0 [00 00 1A]>>"
               "<<PAUSE [14]>>Oh, wait! Boy, am I dense!<<PAUSE [08]>>\n"
               "This buddy of mine runs the\n"
               "shop in <<STR_COUNTRYNAME>>! <<DEMONPC0 [00 00 FE]>><<PAUSE [08]>>\n"
               "Let me give him a jingle!<<BTN>>\n"
               "<<MSGCLEAR>>This'll take me two seconds.<<PAUSE [08]>>\n"
               "Wait right here, OK?\n"
               "<<MSGEND>>");
    printf("[Online] Rover knows %d town(s)\n", s_town_count);
}

/* Called when K.K.'s talk ends. */
void pc_online_intro_kk_done(void) {
    if (pc_msg_take_loaded(KK_ONLINE_MSG)) {
        printf("[Online] heading online\n");
        pc_online_intro_setup_rover();
    }
}

/* Every frame: react to what was picked on the train. */
void pc_online_intro_tick(void) {
    int i;

    if (pc_msg_take_loaded(ROVER_NEW_TOWN_MSG)) {
        printf("[Online] new town: it goes online at its first save\n");
        pc_cloud_defer_new_town();
    }
    for (i = 0; i < s_town_count; i++) {
        if (pc_msg_take_loaded(ROVER_TOWN_MSG(i))) {
            s_chosen_town = i;
        }
    }
    /* Once Rover is done talking, restart the game in that town. */
    if (s_chosen_town >= 0 && mMsg_PC_IsWindowIdle()) {
        printf("[Online] heading to %s\n", s_towns[s_chosen_town].name);
        pc_relaunch_with("--town", s_towns[s_chosen_town].id);
    }
}
