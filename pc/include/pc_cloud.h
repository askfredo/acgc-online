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
