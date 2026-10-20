/* pc_cloud.h - towns saved on the online town server (server/).
 *
 * With --town ID the game hosts that town from the cloud: it takes the town's host
 * lock, downloads the latest save at boot, uploads every save in the background and
 * releases the lock on exit. Account and town commands (--register, --login, --towns,
 * --create-town, --join-town) run from the command line and exit.
 *
 * Settings live in online.ini next to the game (server address, user, session token). */
#ifndef PC_CLOUD_H
#define PC_CLOUD_H

#include <stdint.h>

extern int g_pc_cloud_mode;     /* 1 while playing a cloud town */
extern char g_pc_cloud_town[32]; /* town id */

/* Command line: returns 1 if argv[*i] was a cloud option (and advances *i past its
 * arguments). Options that are commands set *run_command: run pc_cloud_run_command and exit. */
int pc_cloud_parse_arg(int argc, char** argv, int* i, int* run_command);
/* Runs the command given on the command line; returns the process exit code. */
int pc_cloud_run_command(void);

/* --- for the start menu (blocking calls; err gets a readable message) --- */

typedef struct {
    char id[32];
    char name[48];
    char code[16];
    int residents;
    int free_slots;
    int save_version;
    char host[32]; /* resident playing it right now, "" if nobody */
} PcCloudTown;

#define PC_CLOUD_MAX_TOWNS 32

/* Loads online.ini; returns 1 if there is a server and a session. */
int pc_cloud_load_settings(void);
const char* pc_cloud_get_server(void);
const char* pc_cloud_get_user(void);
void pc_cloud_set_server(const char* url);
int pc_cloud_auth(const char* user, const char* pass, int create_account, char* err, int err_size);
void pc_cloud_logout(void);
/* Returns the number of towns, or -1 on error. */
int pc_cloud_list_towns(PcCloudTown* out, int max, char* err, int err_size);
int pc_cloud_create_town(const char* name, PcCloudTown* out, char* err, int err_size);
int pc_cloud_join_town(const char* code, PcCloudTown* out, char* err, int err_size);
/* Play this town from the cloud (pc_cloud_start does the rest). */
void pc_cloud_select_town(const char* id);

/* Players never type an account: one is created on this PC the first time it's needed
 * (random name and password kept in online.ini), on the configured or default server. */
int pc_cloud_ensure_account(char* err, int err_size);

/* A brand-new town started from the intro: it is created on the server, named after the
 * town, the first time the game saves (pc_cloud_on_first_save). */
void pc_cloud_defer_new_town(void);
int pc_cloud_new_town_pending(void);
int pc_cloud_on_first_save(const char* town_name);

/* Playing a cloud town: lock + download (blocks; exits with a message on failure). */
void pc_cloud_start(void);
void pc_cloud_shutdown(void);

/* The downloaded town (NULL for a new town that was never saved). */
const uint8_t* pc_cloud_get_boot_save(uint32_t* size);
void pc_cloud_free_boot_save(void);
/* Queue a save (whole GCI with header) for upload. */
void pc_cloud_queue_upload(const uint8_t* gci, uint32_t size);
/* Local cache of the cloud town's save, used as the save path while playing it. */
const char* pc_cloud_save_dir(void);

#endif /* PC_CLOUD_H */
