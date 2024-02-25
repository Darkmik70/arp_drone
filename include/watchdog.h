#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <sys/types.h>

#define THRESHOLD 7 // Threshold for counters

void get_args(int argc, char *argv[]);

/**
 * Get PIDs from other processes.
 * This function retrieves pids from other processes using shared memory created by the server. 
 * For this reason it uses 3 semaphores make sure no deadlock will happen between processes.
 * When PIDs are retrieves it sends a signal to Server to unlink semaphores and shared memory.
 * 
*/
int get_pids(pid_t *server_pid, pid_t *window_pid, pid_t *km_pid, 
                pid_t *drone_pid, pid_t *obstacles_pid, pid_t *targets_pid);

/**
 * Sends SIGINT signal to all processes and exits afterwards
*/
void send_sigint_to_all();

#endif //WATCHDOG_H