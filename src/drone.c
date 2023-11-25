#include "drone.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>


int main() {
    double x = 100.0;  // Initial position of X
    double Vx = 0.0;    // Initial velocity of X
    double forceX = 10.0; // Applied force in the X direction

    double y = 50.0;   // Initial position of Y
    double Vy = 0.0;    // Initial velocity of Y
    double forceY = 5.0; // Applied force in the Y direction

    // Simulate the motion in an infinite loop using Euler's method
    while (1) {
        eulerMethod(&x, &Vx, forceX, &y, &Vy, forceY);
        printf("X - Position: %.2f / Velocity: %.2f\t|\t", x, Vx);
        printf("Y - Position: %.2f / Velocity: %.2f\n", y, Vy);

        // Introduce a delay to simulate real-time intervals
        usleep(TIME_INTERVAL * 1e6);
    }

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
