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
#include <sys/select.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <math.h>
#include <semaphore.h>
#include <signal.h>
#include <string.h>
#include <errno.h>


// Pipes
int action_fd[2];

// New Constants 
const double Coefficient = 400.0; // This was obtained by trial-error
double minDistance = 2.0;
double startDistance = 5.0;

/* Global variables */
int shm_pos_fd;             // File descriptor for drone position shm
int shm_action_fd;          // File descriptor for actions shm
char *ptr_pos;              // Shared memory for Drone Position 
char *ptr_action;           // Shared memory for drone action 
sem_t *sem_pos;             // semaphore for drone positions
sem_t *sem_action;          // semaphore for drone action




int main() 
{
    struct sigaction sa;
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);
    sigaction (SIGUSR1, &sa, NULL);    

    publish_pid_to_wd(DRONE_SYM, getpid());

    // DELETE: Shared memory for DRONE POSITION
    shm_pos_fd = shm_open(SHM_POS, O_RDWR, 0666);
    ptr_pos = mmap(0, SIZE_SHM, PROT_READ | PROT_WRITE, MAP_SHARED, shm_pos_fd, 0);
    // DELETE: Shared memory for DRONE CONTROL - ACTION
    shm_action_fd = shm_open(SHM_ACTION, O_RDWR, 0666);
    ptr_action = mmap(0, SIZE_SHM, PROT_READ | PROT_WRITE, MAP_SHARED, shm_action_fd, 0);
    // DELETE: semaphores
    sem_pos = sem_open(SEM_POS, 0);
    sem_action = sem_open(SEM_ACTION, 0);

    usleep(SLEEP_DRONE); // To let the interface.c process execute first write the initial positions.

    int x; int y;
    int maxX; int maxY;
    int actionX; int actionY;
    // DELETE: Shared memory read for drone position.
    sscanf(ptr_pos, "%d,%d,%d,%d", &x, &y, &maxX, &maxY); 
    // TODO: Using pipes obtain the initial drone position values and screen dimensions (from interface.c)
    // *

    // Variables for euler method
    double pos_x = (double)x;
    double pos_y = (double)y;
    double Vx = 0.0; double Vy = 0.0;    // Initial velocity
    double forceX = 0.0; double forceY = 0.0; // Initial drone force

    while (1)
    {
        int xi; int yi;
        // DELETE: Reading data from shared memory - Drone Position (from interface.c)
        sscanf(ptr_pos, "%d,%d,%d,%d", &xi, &yi, &maxX, &maxY);
        // TODO: READ the pipe for Drone Position (from interface.c)
        // *

        // DELETE: Reading data from shared memory - Drone Action (from key_manager.c)
        sscanf(ptr_action, "%d,%d", &actionX, &actionY);
        // TODO: READ the pipe for Drone Action (from key_manager.c)
        // *

        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(action_fd[0], &read_fds);

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;

        char action_msg[20];

        int ready = select(action_fd[0] + 1, &read_fds, NULL, NULL, &timeout);
        if (ready == -1) {
            perror("Error in select");
        }

        if (ready > 0 && FD_ISSET(action_fd[0], &read_fds)) {
            char msg[MSG_LEN];
            ssize_t bytes_read = read(action_fd[0], msg, sizeof(msg));
            if (bytes_read > 0) {
                // Null-terminate the received data to make it a valid string
                msg[bytes_read] = '\0';
                // Copy the received data to action_msg
                strncpy(action_msg, msg, sizeof(action_msg));
                // Print the received string
                printf("Received ACTION from pipe: %s\n", action_msg);
                fflush(stdout);
            }
        } else { printf("No data available to be read from the pipe.\n");}
        // END OF TRYING TO ACQUIRE DATA FROM ACTION PIPE FROM (KEY_MANAGER.C)


        double ext_forceX = 0.0; double ext_forceY = 0.0; // Initial external force

        /* Convert the action number read into force for the drone */ 
        if (abs(actionX) <= 1.0) // Only values between -1 to 1 are used to move the drone
        {
        forceX += (double)actionX;
        forceY += (double)actionY;
        /* Capping to the max value of force */
        if (forceX > F_MAX){forceX = F_MAX;}
        if (forceX < -F_MAX){forceX = -F_MAX;}
        if (forceY > F_MAX){forceY = F_MAX;}
        if (forceY < -F_MAX){forceY = -F_MAX;}
        }
        // For now, other values for action represent a STOP command.
        else{forceX = 0.0,forceY = 0.0;}

        /* Calculate the EXTERNAL FORCE from obstacles and targets */

        // TARGETS
        // TODO: Obtain the string for targets_msg from a pipe from (interface.c)
        char targets_msg[] = "140,23";
        double target_x, target_y;
        sscanf(targets_msg, "%lf,%lf", &target_x, &target_y);
        calculateExtForce(pos_x, pos_y, target_x, target_y, 0.0, 0.0, &ext_forceX, &ext_forceY);

        // OBSTACLES
        // TODO: Obtain the string for targets_msg from a pipe from (server.c) that comes originally from (obstacles.c)
        char obstacles_msg[] = "O[7]35,11|100,5|16,30|88,7|130,40|53,15|60,10";
        Obstacles obstacles[30];
        int numObstacles;
        parseObstaclesMsg(obstacles_msg, obstacles, &numObstacles);

        for (int i = 0; i < numObstacles; i++) {
            calculateExtForce(pos_x, pos_y, 0.0, 0.0, obstacles[i].x, obstacles[i].y, &ext_forceX, &ext_forceY);
        }
        

        // Using the euler method to obtain the new position for the drone, according to the forces.
        double maxX_f = (double)maxX;
        double maxY_f = (double)maxY;
        eulerMethod(&pos_x, &Vx, forceX, ext_forceX, &maxX_f);
        eulerMethod(&pos_y, &Vy, forceY, ext_forceY, &maxY_f);

        // Only print the data ONLY when there is movement from the drone.
        if (fabs(Vx) > FLOAT_TOLERANCE || fabs(Vy) > FLOAT_TOLERANCE)
        {
            printf("Drone force (X,Y): %.2f,%.2f\t|\t",forceX,forceY);
            printf("External force (X,Y): %.2f,%.2f\n",ext_forceX,ext_forceY);
            printf("X - Position: %.2f / Velocity: %.2f\t|\t", pos_x, Vx);
            printf("Y - Position: %.2f / Velocity: %.2f\n", pos_y, Vy);
            fflush(stdout);
        }
        // DELETE: Write zeros on action memory
        sprintf(ptr_action, "%d,%d", 0, 0);
        // TODO: Just make sure (from key_manager.c) that once a key is pressed, it does not repeteadly send the same key over and over.
        // *

        int xf = (int)round(pos_x);
        int yf = (int)round(pos_y);
        // DELETE: Write new drone position to shared memory
        sprintf(ptr_pos, "%d,%d,%d,%d", xf, yf, maxX, maxY);
        // TODO: Using pipes write this data so it can be read from (interface.c)

        // DELETE: Semaphore post
        sem_post(sem_pos);
        
        usleep(D_T * 1e6); // 0.1 s
    }

    // DELETE: Close shared memories and semaphores
    close(shm_pos_fd);
    close(shm_action_fd);
    sem_close(sem_pos);
    sem_close(sem_action);

    return 0;
}


void signal_handler(int signo, siginfo_t *siginfo, void *context) 
{
    //printf("Received signal number: %d \n", signo);
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


void calculateExtForce(double droneX, double droneY, double targetX, double targetY,
                        double obstacleX, double obstacleY, double *ext_forceX, double *ext_forceY)
{

    // ***Calculate ATTRACTION force towards the target***
    double distanceToTarget = sqrt(pow(targetX - droneX, 2) + pow(targetY - droneY, 2));
    double angleToTarget = atan2(targetY - droneY, targetX - droneX);

    // To avoid division by zero or extremely high forces
    if (distanceToTarget < minDistance) {
        distanceToTarget = minDistance; // Keep the force value as it were on the minDistance
    }
    // Bellow 5m the attraction force will be calculated.
    else if (distanceToTarget < startDistance){
    *ext_forceX += Coefficient * (1.0 / distanceToTarget - 1.0 / 5.0) * (1.0 / pow(distanceToTarget,2)) * cos(angleToTarget);
    *ext_forceY += Coefficient * (1.0 / distanceToTarget - 1.0 / 5.0) * (1.0 / pow(distanceToTarget,2)) * sin(angleToTarget);
    }
    else{
        *ext_forceX += 0;
        *ext_forceY += 0;  
    }


    // ***Calculate REPULSION force from the obstacle***
    double distanceToObstacle = sqrt(pow(obstacleX - droneX, 2) + pow(obstacleY - droneY, 2));
    double angleToObstacle = atan2(obstacleY - droneY, obstacleX - droneX);

    // To avoid division by zero or extremely high forces
    if (distanceToObstacle < minDistance) {
            distanceToObstacle = minDistance; // Keep the force value as it were on the minDistance
        }
    // Bellow 5m the repulsion force will be calculated
    else if (distanceToObstacle < startDistance){
    *ext_forceX -= Coefficient * (1.0 / distanceToObstacle - 1.0 / 5.0) * (1.0 / pow(distanceToObstacle,2)) * cos(angleToObstacle);
    *ext_forceY -= Coefficient * (1.0 / distanceToObstacle - 1.0 / 5.0) * (1.0 / pow(distanceToObstacle,2)) * sin(angleToObstacle);
    }
    else{
        *ext_forceX -= 0;
        *ext_forceY -= 0;
    }
    // TO FIX A BUG WITH BIG FORCES APPEARING OUT OF NOWHERE
    if(*ext_forceX > 50){*ext_forceX=0;}
    if(*ext_forceY > 50){*ext_forceY=0;}
    if(*ext_forceX < 50){*ext_forceX=0;}
    if(*ext_forceY < 50){*ext_forceY=0;}
}


void parseObstaclesMsg(char *obstacles_msg, Obstacles *obstacles, int *numObstacles)
{
    int totalObstacles;
    sscanf(obstacles_msg, "O[%d]", &totalObstacles);

    char *token = strtok(obstacles_msg + 4, "|");
    *numObstacles = 0;

    while (token != NULL && *numObstacles < totalObstacles) {
        sscanf(token, "%d,%d", &obstacles[*numObstacles].x, &obstacles[*numObstacles].y);
        obstacles[*numObstacles].total = *numObstacles + 1;

        token = strtok(NULL, "|");
        (*numObstacles)++;
    }
}


void eulerMethod(double *pos, double *vel, double force, double extForce, double *maxPos)
{
    double total_force = force + extForce;
    double accelerationX = (total_force - DAMPING * (*vel)) / MASS;
    *vel = *vel + accelerationX * D_T;
    *pos = *pos + (*vel) * D_T;

    // Walls are the maximum position the drone can reach
    if (*pos < 0){*pos = 0;}
    if (*pos > *maxPos){*pos = *maxPos - 1;}
}