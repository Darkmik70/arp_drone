#include "drone.h"
#include "constants.h"
#include "util.h"

#include <stdio.h>       
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>

// #include <ctype.h>
#include <fcntl.h>
#include <math.h>
#include <semaphore.h>
#include <signal.h>


/* Global variables */
int shm_pos_fd;             // File descriptor for drone position shm
int shm_action_fd;          // File descriptor for actions shm
char *ptr_pos;              // Shared memory for Drone Position 
char *ptr_action;           // Shared memory for drone action 
sem_t *sem_pos;             // semaphore for drone positions
sem_t *sem_action;          // semaphore for drone action



void signal_handler(int signo, siginfo_t *siginfo, void *context) 
{
    printf("Received signal number: %d \n", signo);
    if( signo == SIGINT)
    {
        printf("Caught SIGINT \n");
        // close all semaphores
        sem_close(sem_action);

        printf("Succesfully closed all semaphores\n");
        exit(1);
    }
    if (signo == SIGUSR1)
    {
        // Get watchdog's pid
        pid_t wd_pid = siginfo->si_pid;
        // inform on your condition
        kill(wd_pid, SIGUSR2);
        // printf("SIGUSR2 SENT SUCCESSFULLY\n");
    }
}

int main() 
{
    struct sigaction sa;
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);
    sigaction (SIGUSR1, &sa, NULL);    


    publish_pid_to_wd(DRONE_SYM, getpid());

 
    // Shared memory for DRONE POSITION
    shm_pos_fd = shm_open(SHM_POS, O_RDWR, 0666);
    ptr_pos = mmap(0, SIZE_SHM, PROT_READ | PROT_WRITE, MAP_SHARED, shm_pos_fd, 0);

    // Shared memory for DRONE CONTROL - ACTION
    shm_action_fd = shm_open(SHM_ACTION, O_RDWR, 0666);
    ptr_action = mmap(0, SIZE_SHM, PROT_READ | PROT_WRITE, MAP_SHARED, shm_action_fd, 0);

    sem_pos = sem_open(SEM_POS, 0);
    sem_action = sem_open(SEM_ACTION, 0);


    usleep(SLEEP_DRONE); // To let the interface.c process execute first write the initial positions.
    int x; int y;
    int maxX; int maxY;
    int actionX; int actionY;
    sscanf(ptr_pos, "%d,%d,%d,%d", &x, &y, &maxX, &maxY); // Obtain the values of X,Y from shared memory

    // Variables for euler method
    double pos_x = (double)x;
    double pos_y = (double)y;
    double Vx = 0.0;    // Initial velocity of X
    double forceX = 0.0; // Initial force in the X direction
    double Vy = 0.0;    // Initial velocity of Y
    double forceY = 0.0; // Initial force in the Y direction

    bool euler_method = true; // For testing purposes.

    while (1)
    {
        int xi; int yi;
        sscanf(ptr_pos, "%d,%d,%d,%d", &xi, &yi, &maxX, &maxY);
        sscanf(ptr_action, "%d,%d", &actionX, &actionY);

        /* DRONE CONTROL */
        if(euler_method)
        {    
            if (abs(actionX) <= 1.0) // Only values between -1 to 1 are used to move the drone
            {
                forceX += (double)actionX;
                forceY += (double)actionY;

                 /* Capping to the max value of force */
                if (forceX > F_MAX)
                {
                    forceX = F_MAX;
                }
                if (forceX < -F_MAX)
                {
                    forceX = -F_MAX;
                }
                if (forceY > F_MAX)
                {
                    forceY = F_MAX;
                }
                if (forceY < -F_MAX)
                {
                    forceY = -F_MAX;
                }
            }
            // For now, other values for action represent a STOP command.
            else
            {
                forceX = 0.0; 
                forceY = 0.0;
            }

            // Calling the function
            double maxX_f = (double)maxX;
            double maxY_f = (double)maxY;
            eulerMethod(&pos_x, &Vx, forceX, &pos_y, &Vy, forceY, &maxX_f, &maxY_f);
            
            // Only print the positions when there is still velocity present.
            if (fabs(Vx) > FLOAT_TOLERANCE || fabs(Vy) > FLOAT_TOLERANCE)
            {
                printf("Force (X,Y): %.2f,%.2f\n",forceX,forceY);
                printf("X - Position: %.2f / Velocity: %.2f\t|\t", pos_x, Vx);
                printf("Y - Position: %.2f / Velocity: %.2f\n", pos_y, Vy);
                fflush(stdout);
            }
            // Write zeros on action memory
            sprintf(ptr_action, "%d,%d", 0, 0); 

            // Write new drone position to shared memory
            int xf = (int)round(pos_x);
            int yf = (int)round(pos_y);
            sprintf(ptr_pos, "%d,%d,%d,%d", xf, yf, maxX, maxY);

            sem_post(sem_pos);
        }
        // /* DRONE CONTROL WITH THE STEP METHOD*/
        // else
        // {
        //     if (abs(actionX) <= 1.0)
        //     {
        //         // Calling the function
        //         stepMethod(&x,&y,actionX,actionY);
        //         // Only print when there is change in the position.
        //         if(actionX!=0 || actionY!=0){
        //             printf("Action (X,Y): %s\n",ptr_action);
        //             printf("X - Position: %d / Velocity: %.2f\t|\t", x, Vx);
        //             printf("Y - Position: %d / Velocity: %.2f\n", y, Vy);
        //             fflush(stdout);
        //         }
        //         sprintf(ptr_action, "%d,%d", 0, 0); // Zeros written on action memory
        //         // Write new drone position to shared memory
        //         sprintf(ptr_pos, "%d,%d,%d,%d", x, y, maxX, maxY);
        //         sem_post(sem_pos);
        //     }
        // }
        // Introduce a delay on the loop to simulate real-time intervals.
        usleep(TIME_INTERVAL * 1e6); // 0.1 s
    }

    // Close shared memories and semaphores
    close(shm_pos_fd);
    close(shm_action_fd);
    sem_close(sem_pos);
    sem_close(sem_action);

    return 0;
}

void eulerMethod(double *x, double *Vx, double forceX,
                 double *y, double *Vy, double forceY,
                 double *maxX, double *maxY)
{
    double accelerationX = (forceX - DAMPING * (*Vx)) / MASS;
    double accelerationY = (forceY - DAMPING * (*Vy)) / MASS;

    // Update velocity and position for X using Euler's method
    *Vx = *Vx + accelerationX * TIME_INTERVAL;
    *x = *x + (*Vx) * TIME_INTERVAL;
    if (*x < 0){*x = 0;}
    if (*x > *maxX){*x = *maxX-1;}

    // Update velocity and position for Y using Euler's method
    *Vy = *Vy + accelerationY * TIME_INTERVAL;
    *y = *y + (*Vy) * TIME_INTERVAL;
    if (*y < 0){*y = 0;}
    if (*y > *maxY){*y = *maxY-1;}
}


//  void stepMethod(int *x, int *y, int actionX, int actionY)
// {
//     (*x) = (*x) + actionX;
//     (*y) = (*y) + actionY;
// }