#ifndef TARGETS_H
#define TARGETS_H

#include <signal.h>

// User-defined parameters
#define MAX_TARGETS 5

// Structure to represent a single target
typedef struct {
    int x;
    int y;
} Target;  // Different from "targets" in constants.h

/**
 * Obtain the file descriptors of the pipes given by the main process.
*/
void get_args(int argc, char *argv[]);

/**
 * Handles the signals monitored by the watchdog process.
*/
void signal_handler(int signo, siginfo_t *siginfo, void *context);



#endif // TARGETS_H