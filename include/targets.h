#ifndef TARGETS_H
#define TARGETS_H

// User-defined parameters
#define SCREEN_WIDTH 168
#define SCREEN_HEIGHT 44
#define MAX_TARGETS 8

// Structure to represent a target
typedef struct {
    int x;
    int y;
} Target;

void get_args(int argc, char *argv[]);
void generateRandomCoordinates(int sectorWidth, int sectorHeight, int *x, int *y);

#endif // TARGETS_H