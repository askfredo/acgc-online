/* pc_cloud.c - towns saved on the online town server (see pc_cloud.h) */
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
#define PC_CLOUD_RENEW_MS 20000
#define PC_CLOUD_RETRY_MS 5000

int g_pc_cloud_mode = 0;
char g_pc_cloud_town[32] = "";

/* online.ini */
static char s_server[256] = "";
static char s_user[64] = "";
static char s_token[128] = "";

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

static int pc_cloud_auth_command(const char* path) {
    PcHttpResponse resp;
    char user[160];
    char pass[160];
    char body[400];
    char err[160];

    pc_cloud_json_escape(s_arg1, user, sizeof(user));
    pc_cloud_json_escape(s_arg2, pass, sizeof(pass));
    snprintf(body, sizeof(body), "{\"username\":\"%s\",\"password\":\"%s\"}", user, pass);
    s_token[0] = '\0';
    pc_cloud_call("POST", path, NULL, body, (uint32_t)strlen(body), &resp);
    if (resp.status != 200 && resp.status != 201) {
        fprintf(stderr, "Error: %s\n", pc_cloud_error_text(&resp, err, sizeof(err)));
        pc_http_free(&resp);
        return 1;
    }
    pc_cloud_json_str((const char*)resp.body, "username", s_user, sizeof(s_user));
    pc_cloud_json_str((const char*)resp.body, "token", s_token, sizeof(s_token));
    pc_http_free(&resp);
    pc_cloud_save_ini();
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
        size_t n;

        snprintf(s_server, sizeof(s_server), "%s", s_arg1);
        n = strlen(s_server);
        while (n > 0 && s_server[n - 1] == '/') {
            s_server[--n] = '\0';
        }
        pc_cloud_save_ini();
        printf("Server set to %s\n", s_server);
        return 0;
    }
    if (!pc_cloud_need_server()) {
        return 1;
    }

    switch (s_cmd) {
        case PC_CLOUD_CMD_REGISTER:
            return pc_cloud_auth_command("/api/v1/register");
        case PC_CLOUD_CMD_LOGIN:
            return pc_cloud_auth_command("/api/v1/login");
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

void pc_cloud_start(void) {
    PcHttpResponse resp;
    char path[160];
    char msg[256];
    char err[160];

    if (!g_pc_cloud_mode) {
        return;
    }
    pc_cloud_load_ini();
    if (s_server[0] == '\0' || s_token[0] == '\0') {
        pc_cloud_fail("Primero configura el servidor y entra a tu cuenta:\n"
                      "  --server https://tu-servidor.up.railway.app\n  --login usuario contrasena");
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
    } else if (resp.status == 404) {
        s_save_version = 0;
        printf("[Cloud] town %s has no save yet: starting a new town\n", g_pc_cloud_town);
    } else {
        snprintf(msg, sizeof(msg), "No se pudo descargar el pueblo: %s", pc_cloud_error_text(&resp, err, sizeof(err)));
        pc_cloud_lock_call("DELETE", "", &resp);
        pc_cloud_fail(msg);
    }
    pc_http_free(&resp);

    /* Local copy of the cloud town, separate from the offline save. */
    pc_mkdir("cloud");
    snprintf(s_save_dir, sizeof(s_save_dir), "cloud/%s", g_pc_cloud_town);
    pc_mkdir(s_save_dir);

    s_mutex = SDL_CreateMutex();
    s_cond = SDL_CreateCond();
    {
        extern int pc_cloud_worker(void* arg);
        s_worker = SDL_CreateThread(pc_cloud_worker, "cloud", NULL);
    }
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
            if (resp.status != 200) {
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
