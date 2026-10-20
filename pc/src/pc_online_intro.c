/* pc_online_intro.c - the game's own characters lead you to online play.
 *
 * K.K. Slider's first-boot talk ends with "you'll enjoy it more if you get some friends
 * to come here". Here he goes on to ask whether you're heading to a town online with
 * your friends or starting out on your own. Online relaunches the game into the town
 * picker; on your own carries on with the usual intro. */
#include "pc_msg.h"

#include <stdio.h>

/* K.K.'s last first-boot message (player select, no save yet). */
#define KK_FRIENDS_MSG 0x09C9
#define KK_ONLINE_MSG (PC_MSG_BASE + 1)
#define KK_ON_MY_OWN_MSG (PC_MSG_BASE + 2)
#define CHOICE_ONLINE (PC_CHOICE_STR_BASE + 0)
#define CHOICE_ON_MY_OWN (PC_CHOICE_STR_BASE + 1)

extern void pc_relaunch_with(const char* arg);

void pc_online_intro_init(void) {
    int ok = 1;

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

/* Called when K.K.'s talk ends. */
void pc_online_intro_kk_done(void) {
    if (pc_msg_take_loaded(KK_ONLINE_MSG)) {
        printf("[Online] heading online: opening the town picker\n");
        pc_relaunch_with("--menu");
    }
}
