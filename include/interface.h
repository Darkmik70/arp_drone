#ifndef INTERFACE_H
#define INTERFACE_H   

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <ctype.h>
#include <fcntl.h>


//TODO: Add docs
void createBlackboard();
void drawDrone(int droneX, int droneY);
void handleInput(int *sharedKey, sem_t *semaphore);
void initializeDronePosition(int *sharedPos, int dronePosition[2]);


#endif //INTERFACE_H