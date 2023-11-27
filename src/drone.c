#include "drone.h"
#include "constants.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdbool.h>
#include <math.h>


int main() {

    // Initialize shared memory for drone positions
    int sharedPos;
    char *sharedPosition;
    // Initialize shared memory for drone actions.
    int sharedAct;
    char *sharedAction;

    // Shared memory for DRONE POSITION
    sharedPos = shm_open(SHM_POS, O_RDWR, 0666);
    sharedPosition = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, sharedPos, 0);

    // Shared memory for DRONE CONTROL - ACTION
    sharedAct = shm_open(SHM_ACTION, O_RDWR, 0666);
    sharedAction = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, sharedAct, 0);

    sem_t *semaphore_act = sem_open(SHM_ACTION, O_CREAT, 0666, 0);
    if (semaphore_act == SEM_FAILED)
    {
        perror("sem_open");
        //exit(1);
    }

    // Variable declaration segment
    sleep(1); // To let the interface.c process write the initial positions first.
    int x; int y;
    int actionX; int actionY;
    sscanf(sharedPosition, "%d,%d", &x, &y); // Obtain the values of X,Y from shared memory
    printf("%d,%d",x,y);

    // Variables for euler method
    double pos_x = (double)x*2;
    double pos_y = (double)y*2;
    double Vx = 0.0;    // Initial velocity of X
    double forceX = 0.0; // Initial force in the X direction
    double Vy = 0.0;    // Initial velocity of Y
    double forceY = 0.0; // Initial force in the Y direction

    bool euler_method = true; // Obtain from keyboard manager
    // Simulate the motion in an infinite loop using Euler's method
    while (1) {

        sscanf(sharedAction, "%d,%d", &actionX, &actionY);

        if(euler_method){
            /* DRONE CONTROL WITH THE DYNAMICS FORMULA*/
            if(actionX >= -1.0 && actionX <= 1.0){
            forceX += (double)actionX;
            forceY += (double)actionY;
            }
            else{forceX = 0.0; forceY = 0.0;}
            // Maximum force
            double f_max = 20;
            if(forceX>f_max){forceX = f_max;}
            if(forceY>f_max){forceY = f_max;}

            if(forceX<-f_max){forceX = -f_max;}
            if(forceY<-f_max){forceY = -f_max;}

            // Calling the function
            eulerMethod(&pos_x, &Vx, forceX, &pos_y, &Vy, forceY);
            printf("Force (X,Y): %.2f,%.2f\n",forceX,forceY);
            printf("X - Position: %.2f / Velocity: %.2f\t|\t", pos_x, Vx);
            printf("Y - Position: %.2f / Velocity: %.2f\n", pos_y, Vy);
            fflush(stdout);
            sprintf(sharedAction, "%d,%d", 0, 0); // Zeros written on action memory
            // Write new drone position to shared memory
            int xf = (int)round(pos_x);
            int yf = (int)round(pos_y);
            sprintf(sharedPosition, "%d,%d", xf, yf);
        }
        else{
            /* DRONE CONTROL WITH THE STEP METHOD*/
            stepMethod(&x,&y,actionX,actionY);
            printf("Action (X,Y): %s\n",sharedAction);
            printf("X - Position: %d / Velocity: %.2f\t|\t", x, Vx);
            printf("Y - Position: %d / Velocity: %.2f\n", y, Vy);
            fflush(stdout);
            sprintf(sharedAction, "%d,%d", 0, 0); // Zeros written on action memory
            // Write new drone position to shared memory
            sprintf(sharedPosition, "%d,%d", x, y);
            }
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
void stepMethod(int *x, int *y, int actionX, int actionY){
    (*x) = (*x) + actionX;
    (*y) = (*y) + actionY;
}