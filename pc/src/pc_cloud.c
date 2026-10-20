/* pc_cloud.c - towns saved on the online town server (see pc_cloud.h) */
#ifdef _WIN32
#define _CRT_RAND_S /* rand_s, before any CRT header */
#endif
#include "pc_cloud.h"
#include "pc_http.h"

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#define pc_mkdir(p) _mkdir(p)
#else
#define pc_mkdir(p) mkdir((p), 0755)
#endif

#define PC_CLOUD_INI "online.ini"
/* Server used when online.ini doesn't name one. Builds meant for a community server set it
 * with -DPC_CLOUD_DEFAULT_SERVER=\"https://...\". */
#ifndef PC_CLOUD_DEFAULT_SERVER
#define PC_CLOUD_DEFAULT_SERVER ""
#endif
#define PC_CLOUD_RENEW_MS 20000
#define PC_CLOUD_RETRY_MS 5000

int g_pc_cloud_mode = 0;
char g_pc_cloud_town[32] = "";

/* online.ini */
static char s_server[256] = "";
static char s_user[64] = "";
static char s_token[128] = "";
static char s_pass[64] = ""; /* only for the automatic account */
static char s_home[32] = "";  /* home town id ("" = offline save) */
static int s_new_town_pending = 0;
static int s_upload_boot_save = 0; /* the boot save came from this PC: send it up */

enum {
    PC_CLOUD_CMD_NONE,
    PC_CLOUD_CMD_REGISTER,
    PC_CLOUD_CMD_LOGIN,
    PC_CLOUD_CMD_TOWNS,
    PC_CLOUD_CMD_CREATE,
    PC_CLOUD_CMD_JOIN,
    PC_CLOUD_CMD_SET_SERVER,
};
static int s_cmd = PC_CLOUD_CMD_NONE;
static char s_arg1[128];
static char s_arg2[128];

/* session */
static char s_lock_id[64] = "";
static int s_save_version = 0;
static uint8_t* s_boot_save = NULL;
static uint32_t s_boot_save_size = 0;
static char s_save_dir[64] = "";

/* upload worker */
static SDL_Thread* s_worker = NULL;
static SDL_mutex* s_mutex = NULL;
static SDL_cond* s_cond = NULL;
static uint8_t* s_pending = NULL; /* latest save waiting for upload */
static uint32_t s_pending_size = 0;
static int s_quit = 0;
static int s_last_upload_failed = 0;

/* --- settings --- */

static void pc_cloud_load_ini(void) {
    FILE* f = fopen(PC_CLOUD_INI, "r");
    char line[512];

    if (f == NULL) {
        return;
    }
    while (fgets(line, sizeof(line), f)) {
        char* eq = strchr(line, '=');
        char* v;

        if (eq == NULL || line[0] == '#') {
            continue;
        }
        *eq = '\0';
        v = eq + 1;
        v[strcspn(v, "\r\n")] = '\0';
        while (*v == ' ') {
            v++;
        }
        if (strncmp(line, "server", 6) == 0) {
            snprintf(s_server, sizeof(s_server), "%s", v);
        } else if (strncmp(line, "username", 8) == 0) {
            snprintf(s_user, sizeof(s_user), "%s", v);
        } else if (strncmp(line, "token", 5) == 0) {
            snprintf(s_token, sizeof(s_token), "%s", v);
        } else if (strncmp(line, "password", 8) == 0) {
            snprintf(s_pass, sizeof(s_pass), "%s", v);
        } else if (strncmp(line, "home_town", 9) == 0) {
            snprintf(s_home, sizeof(s_home), "%s", v);
        }
    }
    fclose(f);
}

static void pc_cloud_save_ini(void) {
    FILE* f = fopen(PC_CLOUD_INI, "w");

    if (f == NULL) {
        fprintf(stderr, "[Cloud] could not write %s\n", PC_CLOUD_INI);
        return;
    }
    fprintf(f, "# ACGC Online: town server and session (keep the token private)\n");
    fprintf(f, "server = %s\n", s_server);
    fprintf(f, "username = %s\n", s_user);
    fprintf(f, "token = %s\n", s_token);
    if (s_pass[0] != '\0') {
        fprintf(f, "password = %s\n", s_pass);
    }
    if (s_home[0] != '\0') {
        fprintf(f, "home_town = %s\n", s_home);
    }
    fclose(f);
}

/* --- tiny JSON helpers (flat server responses only) --- */

static int pc_cloud_json_str(const char* json, const char* key, char* out, size_t out_size) {
    char pat[64];
    const char* p;
    size_t n = 0;

    snprintf(pat, sizeof(pat), "\"%s\":\"", key);
    p = json != NULL ? strstr(json, pat) : NULL;
    if (p == NULL) {
        return 0;
    }
    p += strlen(pat);
    while (*p != '\0' && *p != '"' && n + 1 < out_size) {
        if (*p == '\\' && p[1] != '\0') {
            p++;
        }
        out[n++] = *p++;
    }
    out[n] = '\0';
    return 1;
}

static int pc_cloud_json_int(const char* json, const char* key, int* out) {
    char pat[64];
    const char* p;

    snprintf(pat, sizeof(pat), "\"%s\":", key);
    p = json != NULL ? strstr(json, pat) : NULL;
    if (p == NULL) {
        return 0;
    }
    *out = atoi(p + strlen(pat));
    return 1;
}

/* Escape a string for a JSON body (quotes and backslashes; other input is plain). */
static void pc_cloud_json_escape(const char* in, char* out, size_t out_size) {
    size_t n = 0;

    for (; *in != '\0' && n + 2 < out_size; in++) {
        if (*in == '"' || *in == '\\') {
            out[n++] = '\\';
        }
        out[n++] = *in;
    }
    out[n] = '\0';
}

/* --- minimal JSON reading for town lists --- */

static const char* pc_json_ws(const char* p) {
    while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') {
        p++;
    }
    return p;
}

/* Skips one value; returns the position after it, or NULL if malformed. */
static const char* pc_json_skip(const char* p) {
    int depth = 0;

    p = pc_json_ws(p);
    do {
        if (*p == '\0') {
            return NULL;
        }
        if (*p == '"') {
            for (p++; *p != '"'; p++) {
                if (*p == '\0') {
                    return NULL;
                }
                if (*p == '\\' && p[1] != '\0') {
                    p++;
                }
            }
            p++;
        } else if (*p == '{' || *p == '[') {
            depth++;
            p++;
        } else if (*p == '}' || *p == ']') {
            depth--;
            p++;
        } else if (depth == 0) {
            while (*p != '\0' && *p != ',' && *p != '}' && *p != ']') {
                p++; /* number, true, false, null */
            }
        } else {
            p++;
        }
        if (depth > 0) {
            p = pc_json_ws(p);
        }
    } while (depth > 0);
    return p;
}

/* In the object starting at obj ('{'), returns the value of key, or NULL. */
static const char* pc_json_get(const char* obj, const char* key) {
    const char* p = pc_json_ws(obj);
    size_t key_len = strlen(key);

    if (*p != '{') {
        return NULL;
    }
    p = pc_json_ws(p + 1);
    while (*p == '"') {
        const char* k = p + 1;
        const char* end = strchr(k, '"');
        int match;

        if (end == NULL) {
            return NULL;
        }
        match = (size_t)(end - k) == key_len && strncmp(k, key, key_len) == 0;
        p = pc_json_ws(end + 1);
        if (*p != ':') {
            return NULL;
        }
        p = pc_json_ws(p + 1);
        if (match) {
            return p;
        }
        p = pc_json_skip(p);
        if (p == NULL) {
            return NULL;
        }
        p = pc_json_ws(p);
        if (*p == ',') {
            p = pc_json_ws(p + 1);
        }
    }
    return NULL;
}

static void pc_json_get_str(const char* obj, const char* key, char* out, size_t out_size) {
    const char* v = pc_json_get(obj, key);
    size_t n = 0;

    out[0] = '\0';
    if (v == NULL || *v != '"') {
        return;
    }
    for (v++; *v != '\0' && *v != '"' && n + 1 < out_size; v++) {
        if (*v == '\\' && v[1] != '\0') {
            v++;
        }
        out[n++] = *v;
    }
    out[n] = '\0';
}

static int pc_json_get_int(const char* obj, const char* key) {
    const char* v = pc_json_get(obj, key);
    return v != NULL ? atoi(v) : 0;
}

static int pc_json_array_len(const char* v) {
    int n = 0;

    v = v != NULL ? pc_json_ws(v) : NULL;
    if (v == NULL || *v != '[') {
        return 0;
    }
    v = pc_json_ws(v + 1);
    while (*v != ']' && *v != '\0') {
        v = pc_json_skip(v);
        if (v == NULL) {
            return n;
        }
        n++;
        v = pc_json_ws(v);
        if (*v == ',') {
            v = pc_json_ws(v + 1);
        }
    }
    return n;
}

static void pc_cloud_parse_town(const char* obj, PcCloudTown* t) {
    const char* host = pc_json_get(obj, "host");

    memset(t, 0, sizeof(*t));
    pc_json_get_str(obj, "id", t->id, sizeof(t->id));
    pc_json_get_str(obj, "name", t->name, sizeof(t->name));
    pc_json_get_str(obj, "code", t->code, sizeof(t->code));
    t->residents = pc_json_array_len(pc_json_get(obj, "members"));
    t->free_slots = pc_json_get_int(obj, "free_slots");
    t->save_version = pc_json_get_int(obj, "save_version");
    if (host != NULL && *host == '{') {
        pc_json_get_str(host, "host", t->host, sizeof(t->host));
    }
}

/* --- requests --- */

static int pc_cloud_call(const char* method, const char* path, const char* const* extra, const void* body,
                         uint32_t body_len, PcHttpResponse* resp) {
    char url[512];
    char auth[192];
    const char* headers[8];
    int n = 0;

    snprintf(url, sizeof(url), "%s%s", s_server, path);
    if (s_token[0] != '\0') {
        snprintf(auth, sizeof(auth), "Authorization: Bearer %s", s_token);
        headers[n++] = auth;
    }
    if (body != NULL && extra == NULL) {
        headers[n++] = "Content-Type: application/json";
    }
    while (extra != NULL && *extra != NULL && n < 7) {
        headers[n++] = *extra++;
    }
    headers[n] = NULL;
    return pc_http_request(method, url, headers, body, body_len, resp);
}

static const char* pc_cloud_error_text(const PcHttpResponse* resp, char* buf, size_t size) {
    if (resp->status == 0) {
        snprintf(buf, size, "%s", resp->error);
    } else if (!pc_cloud_json_str((const char*)resp->body, "error", buf, size)) {
        snprintf(buf, size, "server answered %d", resp->status);
    }
    return buf;
}

static int pc_cloud_need_server(void) {
    if (s_server[0] == '\0') {
        fprintf(stderr, "No server set. Use: --server https://your-server.up.railway.app\n");
        return 0;
    }
    return 1;
}

/* --- command line --- */

int pc_cloud_parse_arg(int argc, char** argv, int* i, int* run_command) {
    const char* a = argv[*i];
    int need = 0;
    int cmd = PC_CLOUD_CMD_NONE;

    if (strcmp(a, "--server") == 0) {
        cmd = PC_CLOUD_CMD_SET_SERVER, need = 1;
    } else if (strcmp(a, "--register") == 0) {
        cmd = PC_CLOUD_CMD_REGISTER, need = 2;
    } else if (strcmp(a, "--login") == 0) {
        cmd = PC_CLOUD_CMD_LOGIN, need = 2;
    } else if (strcmp(a, "--towns") == 0) {
        cmd = PC_CLOUD_CMD_TOWNS;
    } else if (strcmp(a, "--create-town") == 0) {
        cmd = PC_CLOUD_CMD_CREATE, need = 1;
    } else if (strcmp(a, "--join-town") == 0) {
        cmd = PC_CLOUD_CMD_JOIN, need = 1;
    } else if (strcmp(a, "--town") == 0 && *i + 1 < argc) {
        snprintf(g_pc_cloud_town, sizeof(g_pc_cloud_town), "%s", argv[*i + 1]);
        g_pc_cloud_mode = 1;
        (*i)++;
        return 1;
    } else {
        return 0;
    }

    if (*i + need >= argc) {
        fprintf(stderr, "%s needs %d argument(s)\n", a, need);
        exit(2);
    }
    if (need >= 1) {
        snprintf(s_arg1, sizeof(s_arg1), "%s", argv[*i + 1]);
    }
    if (need >= 2) {
        snprintf(s_arg2, sizeof(s_arg2), "%s", argv[*i + 2]);
    }
    *i += need;
    s_cmd = cmd;
    *run_command = 1;
    return 1;
}

/* --- account and towns (command line and start menu) --- */

int pc_cloud_load_settings(void) {
    pc_cloud_load_ini();
    return s_server[0] != '\0' && s_token[0] != '\0';
}

const char* pc_cloud_get_server(void) {
    return s_server;
}

const char* pc_cloud_get_user(void) {
    return s_token[0] != '\0' ? s_user : "";
}

void pc_cloud_set_server(const char* url) {
    size_t n;

    while (*url == ' ') {
        url++;
    }
    snprintf(s_server, sizeof(s_server), "%s", url);
    n = strlen(s_server);
    while (n > 0 && (s_server[n - 1] == '/' || s_server[n - 1] == ' ')) {
        s_server[--n] = '\0';
    }
    /* A bare domain (what Railway shows) means HTTPS. */
    if (n > 0 && strncmp(s_server, "http://", 7) != 0 && strncmp(s_server, "https://", 8) != 0) {
        char tmp[sizeof(s_server)];
        snprintf(tmp, sizeof(tmp), "https://%.240s", s_server);
        snprintf(s_server, sizeof(s_server), "%s", tmp);
    }
    pc_cloud_save_ini();
}

int pc_cloud_auth(const char* user, const char* pass, int create_account, char* err, int err_size) {
    PcHttpResponse resp;
    char user_esc[160];
    char pass_esc[260];
    char body[480];

    pc_cloud_json_escape(user, user_esc, sizeof(user_esc));
    pc_cloud_json_escape(pass, pass_esc, sizeof(pass_esc));
    snprintf(body, sizeof(body), "{\"username\":\"%s\",\"password\":\"%s\"}", user_esc, pass_esc);
    s_token[0] = '\0';
    pc_cloud_call("POST", create_account ? "/api/v1/register" : "/api/v1/login", NULL, body,
                  (uint32_t)strlen(body), &resp);
    if (resp.status != 200 && resp.status != 201) {
        pc_cloud_error_text(&resp, err, err_size);
        pc_http_free(&resp);
        return 0;
    }
    pc_cloud_json_str((const char*)resp.body, "username", s_user, sizeof(s_user));
    pc_cloud_json_str((const char*)resp.body, "token", s_token, sizeof(s_token));
    pc_http_free(&resp);
    pc_cloud_save_ini();
    return 1;
}

void pc_cloud_logout(void) {
    s_token[0] = '\0';
    pc_cloud_save_ini();
}

int pc_cloud_list_towns(PcCloudTown* out, int max, char* err, int err_size) {
    PcHttpResponse resp;
    const char* p;
    int n = 0;

    pc_cloud_call("GET", "/api/v1/towns", NULL, NULL, 0, &resp);
    if (resp.status != 200) {
        pc_cloud_error_text(&resp, err, err_size);
        pc_http_free(&resp);
        return -1;
    }
    p = pc_json_ws((const char*)resp.body);
    if (*p == '[') {
        p = pc_json_ws(p + 1);
        while (*p == '{' && n < max) {
            pc_cloud_parse_town(p, &out[n++]);
            p = pc_json_skip(p);
            if (p == NULL) {
                break;
            }
            p = pc_json_ws(p);
            if (*p == ',') {
                p = pc_json_ws(p + 1);
            }
        }
    }
    pc_http_free(&resp);
    return n;
}

static int pc_cloud_town_call(const char* path, const char* field, const char* value, int ok_status,
                              PcCloudTown* out, char* err, int err_size) {
    PcHttpResponse resp;
    char esc[160];
    char body[256];

    pc_cloud_json_escape(value, esc, sizeof(esc));
    snprintf(body, sizeof(body), "{\"%s\":\"%s\"}", field, esc);
    pc_cloud_call("POST", path, NULL, body, (uint32_t)strlen(body), &resp);
    if (resp.status != ok_status) {
        pc_cloud_error_text(&resp, err, err_size);
        pc_http_free(&resp);
        return 0;
    }
    pc_cloud_parse_town((const char*)resp.body, out);
    pc_http_free(&resp);
    return 1;
}

int pc_cloud_create_town(const char* name, PcCloudTown* out, char* err, int err_size) {
    return pc_cloud_town_call("/api/v1/towns", "name", name, 201, out, err, err_size);
}

int pc_cloud_join_town(const char* code, PcCloudTown* out, char* err, int err_size) {
    return pc_cloud_town_call("/api/v1/towns/join", "code", code, 200, out, err, err_size);
}

void pc_cloud_select_town(const char* id) {
    snprintf(g_pc_cloud_town, sizeof(g_pc_cloud_town), "%s", id);
    g_pc_cloud_mode = 1;
}

static int pc_cloud_auth_command(int create_account) {
    char err[160];

    if (!pc_cloud_auth(s_arg1, s_arg2, create_account, err, sizeof(err))) {
        fprintf(stderr, "Error: %s\n", err);
        return 1;
    }
    printf("Logged in as %s (saved in %s)\n", s_user, PC_CLOUD_INI);
    return 0;
}

static int pc_cloud_simple_command(const char* method, const char* path, const char* body, int ok_status) {
    PcHttpResponse resp;
    char err[160];

    pc_cloud_call(method, path, NULL, body, body ? (uint32_t)strlen(body) : 0, &resp);
    if (resp.status != ok_status && resp.status != 200) {
        fprintf(stderr, "Error: %s\n", pc_cloud_error_text(&resp, err, sizeof(err)));
        pc_http_free(&resp);
        return 1;
    }
    printf("%s\n", (const char*)resp.body);
    pc_http_free(&resp);
    return 0;
}

int pc_cloud_run_command(void) {
    char body[256];
    char esc[160];

    pc_cloud_load_ini();
    if (s_cmd == PC_CLOUD_CMD_SET_SERVER) {
        pc_cloud_set_server(s_arg1);
        printf("Server set to %s\n", s_server);
        return 0;
    }
    if (!pc_cloud_need_server()) {
        return 1;
    }

    switch (s_cmd) {
        case PC_CLOUD_CMD_REGISTER:
            return pc_cloud_auth_command(1);
        case PC_CLOUD_CMD_LOGIN:
            return pc_cloud_auth_command(0);
        case PC_CLOUD_CMD_TOWNS:
            return pc_cloud_simple_command("GET", "/api/v1/towns", NULL, 200);
        case PC_CLOUD_CMD_CREATE:
            pc_cloud_json_escape(s_arg1, esc, sizeof(esc));
            snprintf(body, sizeof(body), "{\"name\":\"%s\"}", esc);
            return pc_cloud_simple_command("POST", "/api/v1/towns", body, 201);
        case PC_CLOUD_CMD_JOIN:
            pc_cloud_json_escape(s_arg1, esc, sizeof(esc));
            snprintf(body, sizeof(body), "{\"code\":\"%s\"}", esc);
            return pc_cloud_simple_command("POST", "/api/v1/towns/join", body, 200);
    }
    return 0;
}

/* --- playing a cloud town --- */

static void pc_cloud_fail(const char* msg) {
    fprintf(stderr, "[Cloud] %s\n", msg);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ACGC Online", msg, NULL);
    exit(1);
}

static int pc_cloud_lock_call(const char* method, const char* suffix, PcHttpResponse* resp) {
    char path[160];
    char lock_hdr[96];
    const char* extra[2] = { lock_hdr, NULL };

    snprintf(path, sizeof(path), "/api/v1/towns/%s/lock%s", g_pc_cloud_town, suffix);
    snprintf(lock_hdr, sizeof(lock_hdr), "X-Lock-Id: %s", s_lock_id);
    return pc_cloud_call(method, path, s_lock_id[0] ? extra : NULL, NULL, 0, resp);
}

/* After taking the lock: local copy of the town and the upload/renew worker. */
static void pc_cloud_open_session(void) {
    extern int pc_cloud_worker(void* arg);

    /* Local copy of the cloud town, separate from the offline save. */
    pc_mkdir("cloud");
    snprintf(s_save_dir, sizeof(s_save_dir), "cloud/%s", g_pc_cloud_town);
    pc_mkdir(s_save_dir);

    s_mutex = SDL_CreateMutex();
    s_cond = SDL_CreateCond();
    s_worker = SDL_CreateThread(pc_cloud_worker, "cloud", NULL);
}

/* Cryptographically secure random bytes from the OS (the account password depends on it). */
static int pc_cloud_random(unsigned char* out, int n) {
#ifdef _WIN32
    int i;
    for (i = 0; i < n; i++) {
        unsigned int v;
        if (rand_s(&v) != 0) {
            return 0;
        }
        out[i] = (unsigned char)v;
    }
    return 1;
#else
    FILE* f = fopen("/dev/urandom", "rb");
    int ok = f != NULL && fread(out, 1, n, f) == (size_t)n;
    if (f != NULL) {
        fclose(f);
    }
    return ok;
#endif
}

int pc_cloud_ensure_account(char* err, int err_size) {
    char user[32];
    char pass[40];
    unsigned char rnd[21];
    int i;

    pc_cloud_load_ini();
    if (s_server[0] == '\0') {
        if (PC_CLOUD_DEFAULT_SERVER[0] == '\0') {
            snprintf(err, err_size, "no town server set (online.ini)");
            return 0;
        }
        pc_cloud_set_server(PC_CLOUD_DEFAULT_SERVER);
    }
    if (s_token[0] != '\0') {
        return 1;
    }
    if (s_user[0] != '\0' && s_pass[0] != '\0' && pc_cloud_auth(s_user, s_pass, 0, err, err_size)) {
        return 1; /* automatic account whose session expired */
    }

    /* New automatic account. The name is only an id; residents keep their in-game names. */
    if (!pc_cloud_random(rnd, sizeof(rnd))) {
        snprintf(err, err_size, "no secure random source");
        return 0;
    }
    snprintf(user, sizeof(user), "p_%02x%02x%02x%02x%02x", rnd[0], rnd[1], rnd[2], rnd[3], rnd[4]);
    for (i = 0; i < 16; i++) {
        snprintf(pass + i * 2, 3, "%02x", rnd[5 + i]);
    }
    if (!pc_cloud_auth(user, pass, 1, err, err_size)) {
        return 0;
    }
    snprintf(s_pass, sizeof(s_pass), "%s", pass);
    pc_cloud_save_ini();
    printf("[Cloud] created this PC's account %s\n", s_user);
    return 1;
}

void pc_cloud_defer_new_town(void) {
    s_new_town_pending = 1;
}

int pc_cloud_new_town_pending(void) {
    return s_new_town_pending;
}

/* The intro's first save: create the town on the server and become its host, so this
 * save (and every later one) goes to the cloud. Returns 1 if the game is now in cloud mode. */
int pc_cloud_on_first_save(const char* town_name) {
    PcCloudTown town;
    PcHttpResponse resp;
    char err[160];

    if (!s_new_town_pending) {
        return 0;
    }
    s_new_town_pending = 0;
    if (!pc_cloud_ensure_account(err, sizeof(err)) ||
        !pc_cloud_create_town(town_name[0] ? town_name : "Town", &town, err, sizeof(err))) {
        printf("[Cloud] could not create the town online: %s (kept offline)\n", err);
        return 0;
    }
    snprintf(g_pc_cloud_town, sizeof(g_pc_cloud_town), "%s", town.id);
    pc_cloud_lock_call("POST", "", &resp);
    if (resp.status != 200 || !pc_cloud_json_str((const char*)resp.body, "lock_id", s_lock_id, sizeof(s_lock_id))) {
        printf("[Cloud] could not host the new town (%d): kept offline\n", resp.status);
        pc_http_free(&resp);
        g_pc_cloud_town[0] = '\0';
        return 0;
    }
    pc_http_free(&resp);
    s_save_version = 0;
    g_pc_cloud_mode = 1;
    pc_cloud_open_session();
    printf("[Cloud] new town %s created online (code %s)\n", town.name, town.code);
    return 1;
}

#define PC_CLOUD_OFFLINE_SAVE "save/DobutsunomoriP_MURA.gci"

/* The offline save on disk (the last time the town was saved before going online). */
static uint8_t* pc_cloud_read_offline_save(uint32_t* size) {
    FILE* f = fopen(PC_CLOUD_OFFLINE_SAVE, "rb");
    uint8_t* buf;
    long n;

    if (f == NULL) {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = (n > 64 && n < 2 * 1024 * 1024) ? (uint8_t*)malloc(n) : NULL;
    if (buf == NULL || fread(buf, 1, n, f) != (size_t)n || memcmp(buf, "GAF", 3) != 0) {
        free(buf);
        fclose(f);
        return NULL;
    }
    fclose(f);
    *size = (uint32_t)n;
    return buf;
}

int pc_cloud_go_online(const char* town_name, char* code, int code_size, char* err, int err_size) {
    PcCloudTown town;
    PcHttpResponse resp;
    char path[96];

    if (g_pc_cloud_mode) {
        /* Already online: look its code up. */
        snprintf(path, sizeof(path), "/api/v1/towns/%s", g_pc_cloud_town);
        pc_cloud_call("GET", path, NULL, NULL, 0, &resp);
        if (resp.status != 200) {
            pc_cloud_error_text(&resp, err, err_size);
            pc_http_free(&resp);
            return 0;
        }
        pc_cloud_parse_town((const char*)resp.body, &town);
        pc_http_free(&resp);
        snprintf(code, code_size, "%s", town.code);
        return 1;
    }

    if (!pc_cloud_ensure_account(err, err_size) ||
        !pc_cloud_create_town(town_name[0] ? town_name : "Town", &town, err, err_size)) {
        return 0;
    }
    snprintf(g_pc_cloud_town, sizeof(g_pc_cloud_town), "%s", town.id);
    pc_cloud_lock_call("POST", "", &resp);
    if (resp.status != 200 || !pc_cloud_json_str((const char*)resp.body, "lock_id", s_lock_id, sizeof(s_lock_id))) {
        pc_cloud_error_text(&resp, err, err_size);
        pc_http_free(&resp);
        g_pc_cloud_town[0] = '\0';
        return 0;
    }
    pc_http_free(&resp);
    s_save_version = 0;
    g_pc_cloud_mode = 1;
    pc_cloud_open_session();
    {
        /* Upload the last save right away, so the town is never empty online even if the
         * game isn't saved again before quitting. */
        uint32_t size;
        uint8_t* save = pc_cloud_read_offline_save(&size);

        if (save != NULL) {
            pc_cloud_queue_upload(save, size);
            free(save);
        }
    }
    /* This town now lives online: open it at start from here on. */
    snprintf(s_home, sizeof(s_home), "%s", town.id);
    pc_cloud_save_ini();
    snprintf(code, code_size, "%s", town.code);
    printf("[Cloud] %s is online now (code %s)\n", town.name, town.code);
    return 1;
}

int pc_cloud_world(PcWorldTown* out, int max, char* err, int err_size) {
    PcHttpResponse resp;
    const char* p;
    int n = 0;

    pc_cloud_call("GET", "/api/v1/world", NULL, NULL, 0, &resp);
    if (resp.status != 200) {
        pc_cloud_error_text(&resp, err, err_size);
        pc_http_free(&resp);
        return -1;
    }
    p = pc_json_ws((const char*)resp.body);
    if (*p == '[') {
        p = pc_json_ws(p + 1);
        while (*p == '{' && n < max) {
            PcWorldTown* t = &out[n++];
            const char* v;

            memset(t, 0, sizeof(*t));
            pc_json_get_str(p, "id", t->id, sizeof(t->id));
            pc_json_get_str(p, "name", t->name, sizeof(t->name));
            t->residents = pc_json_get_int(p, "residents");
            t->free_slots = pc_json_get_int(p, "free_slots");
            v = pc_json_get(p, "playing");
            t->playing = v != NULL && strncmp(v, "true", 4) == 0;
            v = pc_json_get(p, "mine");
            t->mine = v != NULL && strncmp(v, "true", 4) == 0;
            v = pc_json_get(p, "x");
            t->x = v != NULL ? (float)atof(v) : 0.5f;
            v = pc_json_get(p, "y");
            t->y = v != NULL ? (float)atof(v) : 0.5f;

            p = pc_json_skip(p);
            if (p == NULL) {
                break;
            }
            p = pc_json_ws(p);
            if (*p == ',') {
                p = pc_json_ws(p + 1);
            }
        }
    }
    pc_http_free(&resp);
    return n;
}

int pc_cloud_set_public(int is_public, char* err, int err_size) {
    PcHttpResponse resp;
    char path[96];
    const char* body = is_public ? "{\"public\":true}" : "{\"public\":false}";
    int ok;

    if (!g_pc_cloud_mode) {
        snprintf(err, err_size, "this town is not online");
        return 0;
    }
    snprintf(path, sizeof(path), "/api/v1/towns/%s/public", g_pc_cloud_town);
    pc_cloud_call("POST", path, NULL, body, (uint32_t)strlen(body), &resp);
    ok = resp.status == 200;
    if (!ok) {
        pc_cloud_error_text(&resp, err, err_size);
    }
    pc_http_free(&resp);
    return ok;
}

int pc_cloud_move_in(const char* town_id, PcCloudTown* out, char* err, int err_size) {
    PcHttpResponse resp;
    char path[96];

    snprintf(path, sizeof(path), "/api/v1/towns/%s/move-in", town_id);
    pc_cloud_call("POST", path, NULL, "{}", 2, &resp);
    if (resp.status != 200) {
        pc_cloud_error_text(&resp, err, err_size);
        pc_http_free(&resp);
        return 0;
    }
    pc_cloud_parse_town((const char*)resp.body, out);
    pc_http_free(&resp);
    return 1;
}

const char* pc_cloud_home_town(void) {
    return s_home;
}

void pc_cloud_use_home_town(void) {
    pc_cloud_load_ini();
    if (!g_pc_cloud_mode && s_home[0] != '\0') {
        pc_cloud_select_town(s_home);
    }
}

void pc_cloud_start(void) {
    PcHttpResponse resp;
    char path[160];
    char msg[256];
    char err[160];

    if (!g_pc_cloud_mode) {
        return;
    }
    if (!pc_cloud_ensure_account(err, sizeof(err))) {
        snprintf(msg, sizeof(msg), "No se pudo conectar al servidor de pueblos: %s", err);
        pc_cloud_fail(msg);
    }

    /* Become the host of the town. */
    pc_cloud_lock_call("POST", "", &resp);
    if (resp.status == 409) {
        char host[64] = "?";
        pc_cloud_json_str((const char*)resp.body, "host", host, sizeof(host));
        snprintf(msg, sizeof(msg),
                 "%s esta jugando este pueblo ahora.\nPara jugar juntos, unete a su partida (--join).", host);
        pc_cloud_fail(msg);
    }
    if (resp.status != 200 || !pc_cloud_json_str((const char*)resp.body, "lock_id", s_lock_id, sizeof(s_lock_id))) {
        snprintf(msg, sizeof(msg), "No se pudo abrir el pueblo: %s", pc_cloud_error_text(&resp, err, sizeof(err)));
        pc_cloud_fail(msg);
    }
    pc_http_free(&resp);

    /* Latest save (404 = new town, the game starts the intro). */
    snprintf(path, sizeof(path), "/api/v1/towns/%s/save", g_pc_cloud_town);
    pc_cloud_call("GET", path, NULL, NULL, 0, &resp);
    if (resp.status == 200) {
        s_boot_save = resp.body;
        s_boot_save_size = resp.body_len;
        s_save_version = atoi(resp.save_version);
        resp.body = NULL;
        printf("[Cloud] town %s downloaded (version %d)\n", g_pc_cloud_town, s_save_version);
    } else if (resp.status == 404 && strcmp(g_pc_cloud_town, s_home) == 0 &&
               (s_boot_save = pc_cloud_read_offline_save(&s_boot_save_size)) != NULL) {
        /* Home town that went online but never got its save up: use the one on this PC. */
        s_save_version = 0;
        s_upload_boot_save = 1;
        printf("[Cloud] home town %s is empty online: using this PC's save\n", g_pc_cloud_town);
    } else if (resp.status == 404) {
        s_save_version = 0;
        printf("[Cloud] town %s has no save yet: starting a new town\n", g_pc_cloud_town);
    } else {
        snprintf(msg, sizeof(msg), "No se pudo descargar el pueblo: %s", pc_cloud_error_text(&resp, err, sizeof(err)));
        pc_cloud_lock_call("DELETE", "", &resp);
        pc_cloud_fail(msg);
    }
    pc_http_free(&resp);

    pc_cloud_open_session();
    if (s_upload_boot_save) {
        pc_cloud_queue_upload(s_boot_save, s_boot_save_size);
        s_upload_boot_save = 0;
    }
}

/* The town lock was lost (e.g. it expired while the network was down). Take it back, but
 * only if nobody is hosting: another session, even one of ours, keeps the town. */
static int pc_cloud_reclaim_lock(void) {
    PcHttpResponse resp;
    PcCloudTown town;
    char path[96];

    snprintf(path, sizeof(path), "/api/v1/towns/%s", g_pc_cloud_town);
    pc_cloud_call("GET", path, NULL, NULL, 0, &resp);
    if (resp.status != 200) {
        pc_http_free(&resp);
        return 0;
    }
    pc_cloud_parse_town((const char*)resp.body, &town);
    pc_http_free(&resp);
    if (town.host[0] != '\0') {
        printf("[Cloud] %s is hosting this town now: not taking it back\n", town.host);
        return 0;
    }

    s_lock_id[0] = '\0';
    pc_cloud_lock_call("POST", "", &resp);
    if (resp.status == 200 && pc_cloud_json_str((const char*)resp.body, "lock_id", s_lock_id, sizeof(s_lock_id))) {
        pc_http_free(&resp);
        printf("[Cloud] took the town lock back\n");
        return 1;
    }
    pc_http_free(&resp);
    return 0;
}

/* Upload the pending save; on success it is cleared. Returns 0 if it must be retried.
 * Called on the worker thread (and once more at shutdown). */
static int pc_cloud_upload_pending(void) {
    uint8_t* data;
    uint32_t size;
    PcHttpResponse resp;
    char path[160];
    char lock_hdr[96];
    char version_hdr[48];
    const char* extra[4];

    SDL_LockMutex(s_mutex);
    data = s_pending;
    size = s_pending_size;
    s_pending = NULL;
    SDL_UnlockMutex(s_mutex);
    if (data == NULL) {
        return 1;
    }

    snprintf(path, sizeof(path), "/api/v1/towns/%s/save", g_pc_cloud_town);
    snprintf(lock_hdr, sizeof(lock_hdr), "X-Lock-Id: %s", s_lock_id);
    snprintf(version_hdr, sizeof(version_hdr), "X-Save-Version: %d", s_save_version);
    extra[0] = lock_hdr;
    extra[1] = version_hdr;
    extra[2] = "Content-Type: application/octet-stream";
    extra[3] = NULL;
    pc_cloud_call("PUT", path, extra, data, size, &resp);
    if (resp.status == 409 && strstr((const char*)resp.body, "not hosting") != NULL && pc_cloud_reclaim_lock()) {
        pc_http_free(&resp);
        snprintf(lock_hdr, sizeof(lock_hdr), "X-Lock-Id: %s", s_lock_id);
        pc_cloud_call("PUT", path, extra, data, size, &resp);
    }

    if (resp.status == 200) {
        int v;
        if (pc_cloud_json_int((const char*)resp.body, "save_version", &v)) {
            s_save_version = v;
        }
        s_last_upload_failed = 0;
        printf("[Cloud] town saved to the cloud (version %d)\n", s_save_version);
        free(data);
    } else {
        char err[160];
        printf("[Cloud] upload failed: %s\n", pc_cloud_error_text(&resp, err, sizeof(err)));
        s_last_upload_failed = 1;
        SDL_LockMutex(s_mutex);
        if (s_pending == NULL && resp.status != 409) {
            /* Network trouble: keep it for a retry unless a newer save replaced it. */
            s_pending = data;
            s_pending_size = size;
        } else {
            free(data);
        }
        SDL_UnlockMutex(s_mutex);
    }
    pc_http_free(&resp);
    return !s_last_upload_failed;
}

int pc_cloud_worker(void* arg) {
    uint32_t last_renew = SDL_GetTicks();
    (void)arg;

    for (;;) {
        int quit;
        int have_pending;

        SDL_LockMutex(s_mutex);
        if (!s_quit && s_pending == NULL) {
            SDL_CondWaitTimeout(s_cond, s_mutex, PC_CLOUD_RETRY_MS);
        }
        quit = s_quit;
        have_pending = s_pending != NULL;
        SDL_UnlockMutex(s_mutex);

        if (have_pending && !pc_cloud_upload_pending() && !quit) {
            SDL_Delay(PC_CLOUD_RETRY_MS);
        }
        if (quit) {
            break;
        }
        if (SDL_GetTicks() - last_renew >= PC_CLOUD_RENEW_MS) {
            PcHttpResponse resp;

            pc_cloud_lock_call("POST", "/renew", &resp);
            if (resp.status == 409) {
                printf("[Cloud] the town lock was lost\n");
                pc_cloud_reclaim_lock();
            } else if (resp.status != 200) {
                printf("[Cloud] could not renew the town lock (%d)\n", resp.status);
            }
            pc_http_free(&resp);
            last_renew = SDL_GetTicks();
        }
    }
    return 0;
}

void pc_cloud_queue_upload(const uint8_t* gci, uint32_t size) {
    uint8_t* copy;

    if (!g_pc_cloud_mode || s_mutex == NULL) {
        return;
    }
    copy = (uint8_t*)malloc(size);
    if (copy == NULL) {
        return;
    }
    memcpy(copy, gci, size);
    SDL_LockMutex(s_mutex);
    free(s_pending); /* only the newest save matters */
    s_pending = copy;
    s_pending_size = size;
    SDL_CondSignal(s_cond);
    SDL_UnlockMutex(s_mutex);
}

void pc_cloud_shutdown(void) {
    PcHttpResponse resp;

    if (!g_pc_cloud_mode || s_worker == NULL) {
        return;
    }

    /* Let the worker finish the last upload, then give the town back. */
    SDL_LockMutex(s_mutex);
    s_quit = 1;
    SDL_CondSignal(s_cond);
    SDL_UnlockMutex(s_mutex);
    SDL_WaitThread(s_worker, NULL);
    s_worker = NULL;

    if (s_pending != NULL) {
        /* One last try on this thread (the worker gave up on a network error). */
        pc_cloud_upload_pending();
    }
    pc_cloud_lock_call("DELETE", "", &resp);
    pc_http_free(&resp);

    if (s_last_upload_failed) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "ACGC Online",
                                 "La ultima partida no se pudo subir a la nube.\n"
                                 "Quedo guardada en esta PC (carpeta cloud/).",
                                 NULL);
    }
}

const uint8_t* pc_cloud_get_boot_save(uint32_t* size) {
    *size = s_boot_save_size;
    return s_boot_save;
}

void pc_cloud_free_boot_save(void) {
    free(s_boot_save);
    s_boot_save = NULL;
    s_boot_save_size = 0;
}

const char* pc_cloud_save_dir(void) {
    return s_save_dir;
}
