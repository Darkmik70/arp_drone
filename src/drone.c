#include "drone.h"
#include "constants.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <ctype.h>
#include <fcntl.h>


int main() {

    // Initialize shared memory for drone positions
    int sharedPos;
    char *sharedPosition;

    if ((sharedPos = shmget(SHM_KEY_2, 80*sizeof(char), IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(1);
    }

    if ((sharedPosition = shmat(sharedPos, NULL, 0)) == (char *)-1) {
        perror("shmat");
        exit(1);
    }

    
    // Initialize shared memory for drone actions.
    int sharedAct;
    char *sharedAction;

    if ((sharedAct = shmget(SHM_KEY_3, 80*sizeof(char), IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        //exit(1);
    }

    if ((sharedAction = shmat(sharedAct, NULL, 0)) == (char *)-1) {
        perror("shmat");
        //exit(1);
    }

    sem_t *semaphore_act = sem_open(SEM_KEY_3, O_CREAT, 0666, 0);
    if (semaphore_act == SEM_FAILED)
    {
        perror("sem_open");
        //exit(1);
    }

    // Variable declaration segment
    int x; int y;
    int actionX; int actionY;
    sleep(1);
    sscanf(sharedPosition, "%d,%d", &x, &y); // Obtain the values of X,Y from shared memory
    //x = (float)x;
    //y = (float)y;

    double Vx = 0.0;    // Initial velocity of X
    double forceX = 1.0; // Applied force in the X direction

    double Vy = 0.0;    // Initial velocity of Y
    double forceY = 1.0; // Applied force in the Y direction


    // Simulate the motion in an infinite loop using Euler's method
    while (1) {
        //eulerMethod(&x, &Vx, forceX, &y, &Vy, forceY);
        //printf("X - Position: %.2f / Velocity: %.2f\t|\t", x, Vx);
        //printf("Y - Position: %.2f / Velocity: %.2f\n", y, Vy);
        
        sscanf(sharedAction, "%d,%d", &actionX, &actionY);
        printf("Action (X,Y): %s\n",sharedAction);

        stepMethod(&x,&y,actionX,actionY,sharedAction);
        printf("X - Position: %d / Velocity: %.2f\t|\t", x, Vx);
        printf("Y - Position: %d / Velocity: %.2f\n", y, Vy);

        // Write new drone position to shared memory
        sprintf(sharedPosition, "%d,%d", x, y);

        // Introduce a delay to simulate real-time intervals
        usleep(TIME_INTERVAL * 1e6);
    }

    // Detach the shared memory segment
    shmdt(sharedPosition);
    shmdt(sharedAction);

    return 0;
}

// Implementation of the eulerMethod function
void eulerMethod(double *x, double *Vx, double forceX, double *y, double *Vy, double forceY) {
    double accelerationX = (forceX - DAMPING * (*Vx)) / MASS;
    double accelerationY = (forceY - DAMPING * (*Vy)) / MASS;

    // Update velocity and position for X using Euler's method
    *Vx = *Vx + accelerationX * TIME_INTERVAL;
    *x = *x + (*Vx) * TIME_INTERVAL;

    // Update velocity and position for Y using Euler's method
    *Vy = *Vy + accelerationY * TIME_INTERVAL;
    *y = *y + (*Vy) * TIME_INTERVAL;
}

// Moving the drone step by step as initial development
void stepMethod(int *x, int *y, int actionX, int actionY, char *sharedAction){
    (*x) = (*x) + actionX;
    (*y) = (*y) + actionY;

    sprintf(sharedAction, "%d,%d", 0, 0); // Zeros written on action memory
}