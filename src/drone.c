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
        exit(1);
    }

    // Variable declaration segment
    usleep(100000); // To let the interface.c process write the initial positions first.
    int x; int y;
    int maxX; int maxY;
    int actionX; int actionY;
    sscanf(sharedPosition, "%d,%d,%d,%d", &x, &y, &maxX, &maxY); // Obtain the values of X,Y from shared memory

    // Variables for euler method
    double pos_x = (double)x;
    double pos_y = (double)y;
    double Vx = 0.0;    // Initial velocity of X
    double forceX = 0.0; // Initial force in the X direction
    double Vy = 0.0;    // Initial velocity of Y
    double forceY = 0.0; // Initial force in the Y direction

    bool euler_method = true; // Obtain from keyboard manager
    // Simulate the motion in an infinite loop using Euler's method
    while (1) {
        int xi; int yi;
        sscanf(sharedPosition, "%d,%d,%d,%d", &xi, &yi, &maxX, &maxY);
        sscanf(sharedAction, "%d,%d", &actionX, &actionY);

        /* DRONE CONTROL WITH THE DYNAMICS FORMULA*/
        if(euler_method){
            // Only values between -1 to 1 are used to move the drone
            if(actionX >= -1.0 && actionX <= 1.0){
                forceX += (double)actionX;
                forceY += (double)actionY;
                // Capping the force to a maximum value
                double f_max = 20;
                if(forceX>f_max){forceX = f_max;}
                if(forceY>f_max){forceY = f_max;}
                if(forceX<-f_max){forceX = -f_max;}
                if(forceY<-f_max){forceY = -f_max;}
            }
            // For now, other values for action represent a STOP command.
            else{
                forceX = 0.0; 
                forceY = 0.0;
            }
            // Calling the function
            double maxX_f = (double)maxX;
            double maxY_f = (double)maxY;
            eulerMethod(&pos_x, &Vx, forceX, &pos_y, &Vy, forceY, &maxX_f, &maxY_f);
            // Only print the positions when there is still velocity present.
            if(fabs(Vx) > FLOAT_TOLERANCE || fabs(Vy) > FLOAT_TOLERANCE){
                printf("Force (X,Y): %.2f,%.2f\n",forceX,forceY);
                printf("X - Position: %.2f / Velocity: %.2f\t|\t", pos_x, Vx);
                printf("Y - Position: %.2f / Velocity: %.2f\n", pos_y, Vy);
                fflush(stdout);
            }
            // Write zeros on action memory
            sprintf(sharedAction, "%d,%d", 0, 0); 
            // Write new drone position to shared memory
            int xf = (int)round(pos_x);
            int yf = (int)round(pos_y);
            sprintf(sharedPosition, "%d,%d,%d,%d", xf, yf, maxX, maxY);
        }

        /* DRONE CONTROL WITH THE STEP METHOD*/
        else{
            if(actionX >= -1.0 && actionX <= 1.0){
                // Calling the function
                stepMethod(&x,&y,actionX,actionY);
                // Only print when there is change in the position.
                if(actionX!=0 || actionY!=0){
                    printf("Action (X,Y): %s\n",sharedAction);
                    printf("X - Position: %d / Velocity: %.2f\t|\t", x, Vx);
                    printf("Y - Position: %d / Velocity: %.2f\n", y, Vy);
                    fflush(stdout);
                }
                sprintf(sharedAction, "%d,%d", 0, 0); // Zeros written on action memory
                // Write new drone position to shared memory
                sprintf(sharedPosition, "%d,%d,%d,%d", x, y, maxX, maxY);
            }
        }

        // Introduce a delay on the loop to simulate real-time intervals.
        usleep(TIME_INTERVAL * 1e6);
    }

    // Detach the shared memory segment
    shmdt(sharedPosition);
    shmdt(sharedAction);

    return 0;
}

// Implementation of the eulerMethod function
void eulerMethod(double *x, double *Vx, double forceX, double *y, double *Vy, double forceY, double *maxX, double *maxY) {
    double accelerationX = (forceX - DAMPING * (*Vx)) / MASS;
    double accelerationY = (forceY - DAMPING * (*Vy)) / MASS;

    // Update velocity and position for X using Euler's method
    *Vx = *Vx + accelerationX * TIME_INTERVAL;
    *x = *x + (*Vx) * TIME_INTERVAL;
    if (*x < 0){*x = 0;}
    if (*x > *maxX){*x = *maxX;}

    // Update velocity and position for Y using Euler's method
    *Vy = *Vy + accelerationY * TIME_INTERVAL;
    *y = *y + (*Vy) * TIME_INTERVAL;
    if (*y < 0){*y = 0;}
    if (*y > *maxY){*y = *maxY;}
}

// Moving the drone step by step as initial development
void stepMethod(int *x, int *y, int actionX, int actionY){
    (*x) = (*x) + actionX;
    (*y) = (*y) + actionY;
}