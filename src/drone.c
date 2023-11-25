#include "drone.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>  // Added for exit function
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>


int main() {

    // Initialize shared memory for drone positions
    int sharedPos;
    int *sharedPosition;

    if ((sharedPos = shmget(SHM_KEY_2, 2 * sizeof(int), IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(1);
    }

    if ((sharedPosition = shmat(sharedPos, NULL, 0)) == (int *)-1) {
        perror("shmat");
        exit(1);
    }

    // Variable declaration segment

    double x = sharedPosition[0]; // Initial position of X. OBTAINED FROM INTERFACE.C
    double Vx = 0.0;    // Initial velocity of X
    double forceX = 0.1; // Applied force in the X direction

    double y = sharedPosition[1];   // Initial position of Y. OBTAINED FROM INTERFACE.C
    double Vy = 0.0;    // Initial velocity of Y
    double forceY = 0.1; // Applied force in the Y direction

    // Simulate the motion in an infinite loop using Euler's method
    while (1) {
        eulerMethod(&x, &Vx, forceX, &y, &Vy, forceY);
        printf("X - Position: %.2f / Velocity: %.2f\t|\t", x, Vx);
        printf("Y - Position: %.2f / Velocity: %.2f\n", y, Vy);

        // Write new drone position to shared memory
        sharedPosition[0] = x;
        sharedPosition[1] = y;

        // Introduce a delay to simulate real-time intervals
        usleep(TIME_INTERVAL * 1e6);
    }

    // Detach the shared memory segment
    shmdt(sharedPosition);

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