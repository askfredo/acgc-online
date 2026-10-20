/* pc_msg.h - the PC port's own dialogue text, injected into the game's message system.
 *
 * Messages and choice strings with numbers in the PC ranges below never exist on the
 * disc; the game asks pc_msg for them instead (see mMsg_LoadMsgData and
 * mChoice_Load_ChoseStringFromRom). Text is written in UTF-8 with control codes in
 * tools/msg_tool.py syntax, e.g. "Hi!<<BTN>><<MSGCLEAR>>Bye.<<MSGEND>>". */
#ifndef PC_MSG_H
#define PC_MSG_H

#include <stdint.h>

#define PC_MSG_BASE 0x7000 /* message numbers 0x7000-0x71FF */
#define PC_MSG_COUNT 0x200
#define PC_CHOICE_STR_BASE 0x700 /* choice string numbers 0x700-0x77F */
#define PC_CHOICE_STR_COUNT 0x80

#define PC_MSG_IS_CUSTOM(no) ((no) >= PC_MSG_BASE && (no) < PC_MSG_BASE + PC_MSG_COUNT)
#define PC_CHOICE_STR_IS_CUSTOM(no) ((no) >= PC_CHOICE_STR_BASE && (no) < PC_CHOICE_STR_BASE + PC_CHOICE_STR_COUNT)

typedef struct {
    const char* name;
    int size;
} PcMsgControl;

extern const char* const pc_msg_char_map[256];
extern const PcMsgControl pc_msg_controls[];
extern const int pc_msg_control_count;

/* Encode UTF-8 text (with <<CODE [hex]>> control codes) into the game's format.
 * Returns the encoded length, or -1 on an unknown character or code (logged). */
int pc_msg_encode(const char* text, uint8_t* out, int out_max);

/* Make outside text (e.g. a town name from the server) safe to embed in a message:
 * characters missing from the game's font become '?', and "<<" can't start a code. */
void pc_msg_sanitize(const char* in, char* out, int out_size);
/* Decode text in the game's character set (e.g. the town name in a save) to UTF-8. */
void pc_msg_decode(const uint8_t* in, int len, char* out, int out_size);

/* Set the text of a PC message or choice string (copied and encoded now). */
int pc_msg_set(int msg_no, const char* text);
int pc_msg_set_choice(int str_no, const char* text);

/* Replace the text of a message that exists on the disc (by its number). */
int pc_msg_override(int msg_no, const char* text);

/* Called by the game each time a message is loaded into a window. */
void pc_msg_on_loaded(int msg_no);
/* Set when a message is loaded, cleared when read: lets code react to a dialogue branch. */
int pc_msg_take_loaded(int msg_no);

/* Used by the game's loaders: encoded data of a PC message / choice string, or of an
 * overridden disc message. */
int pc_msg_get(int msg_no, const uint8_t** data, uint32_t* size);
int pc_msg_get_choice(int str_no, const uint8_t** data, uint32_t* size);

#endif /* PC_MSG_H */
