#ifndef TARGETS_H
#define TARGETS_H

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


#endif // TARGETS_H