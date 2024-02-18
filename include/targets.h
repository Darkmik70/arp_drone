#ifndef TARGETS_H
#define TARGETS_H

// User-defined parameters
#define MAX_TARGETS 1

// Structure to represent a single target
typedef struct {
    int x;
    int y;
} Target;  // Different from "targets" in constants.h

void get_args(int argc, char *argv[]);
void generateRandomCoordinates(int sectorWidth, int sectorHeight, int *x, int *y);

#endif // TARGETS_H