/* pc_msg.c - the PC port's own dialogue text (see pc_msg.h) */
#include "pc_msg.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PC_MSG_MAX_ENCODED 0x600 /* game message buffers hold a bit more than this */

typedef struct {
    uint8_t* data;
    uint32_t size;
} PcMsgEntry;

#define PC_MSG_MAX_OVERRIDES 16

static PcMsgEntry s_msgs[PC_MSG_COUNT];
static PcMsgEntry s_choices[PC_CHOICE_STR_COUNT];
static struct {
    int msg_no;
    PcMsgEntry entry;
} s_overrides[PC_MSG_MAX_OVERRIDES];
static int s_override_count = 0;
static uint8_t s_loaded[PC_MSG_COUNT]; /* PC messages loaded since last pc_msg_take_loaded */

static int pc_msg_hex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* <<NAME [hex bytes] [more hex]>> starting after "<<"; returns chars consumed or -1. */
static int pc_msg_encode_control(const char* p, uint8_t* out, int* out_len, int out_max) {
    const char* end = strstr(p, ">>");
    char name[32];
    int name_len = 0;
    uint8_t args[16];
    int nargs = 0;
    const char* q = p;
    int i;

    if (end == NULL) {
        return -1;
    }
    while (q < end && *q != ' ' && *q != '[' && name_len < (int)sizeof(name) - 1) {
        name[name_len++] = *q++;
    }
    name[name_len] = '\0';
    while (q < end) {
        int hi;
        int lo;

        if (*q == ' ' || *q == '[' || *q == ']') {
            q++;
            continue;
        }
        hi = pc_msg_hex(q[0]);
        lo = pc_msg_hex(q[1]);
        if (hi < 0 || lo < 0 || nargs >= (int)sizeof(args)) {
            return -1;
        }
        args[nargs++] = (uint8_t)(hi << 4 | lo);
        q += 2;
    }

    for (i = 0; i < pc_msg_control_count; i++) {
        if (pc_msg_controls[i].name != NULL && strcmp(pc_msg_controls[i].name, name) == 0) {
            if (nargs != pc_msg_controls[i].size - 2 || *out_len + 2 + nargs > out_max) {
                return -1;
            }
            out[(*out_len)++] = 0x7F;
            out[(*out_len)++] = (uint8_t)i;
            memcpy(out + *out_len, args, nargs);
            *out_len += nargs;
            return (int)(end - p) + 2;
        }
    }
    return -1;
}

int pc_msg_encode(const char* text, uint8_t* out, int out_max) {
    const char* p = text;
    int len = 0;

    while (*p != '\0') {
        int best = -1;
        size_t best_len = 0;
        int i;

        if (p[0] == '<' && p[1] == '<') {
            int used = pc_msg_encode_control(p + 2, out, &len, out_max);
            if (used < 0) {
                fprintf(stderr, "[Msg] bad control code at: %.40s\n", p);
                return -1;
            }
            p += 2 + used;
            continue;
        }
        /* Longest character of the game's set that matches here (first one on ties). */
        for (i = 0; i < 256; i++) {
            const char* c = pc_msg_char_map[i];
            size_t n = strlen(c);
            if (n > best_len && strncmp(p, c, n) == 0) {
                best = i;
                best_len = n;
            }
        }
        if (best < 0 || len >= out_max) {
            fprintf(stderr, "[Msg] character not in the game's font at: %.20s\n", p);
            return -1;
        }
        out[len++] = (uint8_t)best;
        p += best_len;
    }
    return len;
}

static int pc_msg_store(PcMsgEntry* e, const char* text) {
    uint8_t buf[PC_MSG_MAX_ENCODED];
    int n = pc_msg_encode(text, buf, sizeof(buf));
    uint8_t* copy;

    if (n < 0) {
        return 0;
    }
    copy = (uint8_t*)malloc(n > 0 ? n : 1);
    if (copy == NULL) {
        return 0;
    }
    memcpy(copy, buf, n);
    free(e->data);
    e->data = copy;
    e->size = (uint32_t)n;
    return 1;
}

int pc_msg_set(int msg_no, const char* text) {
    return PC_MSG_IS_CUSTOM(msg_no) && pc_msg_store(&s_msgs[msg_no - PC_MSG_BASE], text);
}

int pc_msg_set_choice(int str_no, const char* text) {
    return PC_CHOICE_STR_IS_CUSTOM(str_no) && pc_msg_store(&s_choices[str_no - PC_CHOICE_STR_BASE], text);
}

static int pc_msg_lookup(PcMsgEntry* e, const uint8_t** data, uint32_t* size) {
    if (e->data == NULL) {
        return 0;
    }
    *data = e->data;
    *size = e->size;
    return 1;
}

int pc_msg_override(int msg_no, const char* text) {
    int i;

    for (i = 0; i < s_override_count; i++) {
        if (s_overrides[i].msg_no == msg_no) {
            return pc_msg_store(&s_overrides[i].entry, text);
        }
    }
    if (s_override_count >= PC_MSG_MAX_OVERRIDES || !pc_msg_store(&s_overrides[s_override_count].entry, text)) {
        return 0;
    }
    s_overrides[s_override_count++].msg_no = msg_no;
    return 1;
}

int pc_msg_get(int msg_no, const uint8_t** data, uint32_t* size) {
    int i;

    if (PC_MSG_IS_CUSTOM(msg_no)) {
        return pc_msg_lookup(&s_msgs[msg_no - PC_MSG_BASE], data, size);
    }
    for (i = 0; i < s_override_count; i++) {
        if (s_overrides[i].msg_no == msg_no) {
            return pc_msg_lookup(&s_overrides[i].entry, data, size);
        }
    }
    return 0;
}

void pc_msg_on_loaded(int msg_no) {
    if (PC_MSG_IS_CUSTOM(msg_no)) {
        s_loaded[msg_no - PC_MSG_BASE] = 1;
    }
}

int pc_msg_take_loaded(int msg_no) {
    int was;

    if (!PC_MSG_IS_CUSTOM(msg_no)) {
        return 0;
    }
    was = s_loaded[msg_no - PC_MSG_BASE];
    s_loaded[msg_no - PC_MSG_BASE] = 0;
    return was;
}

int pc_msg_get_choice(int str_no, const uint8_t** data, uint32_t* size) {
    return PC_CHOICE_STR_IS_CUSTOM(str_no) && pc_msg_lookup(&s_choices[str_no - PC_CHOICE_STR_BASE], data, size);
}
