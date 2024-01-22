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


// Pipes working with the server
int server_drone[2];
int drone_server[2];


int main(int argc, char *argv[]) 
{
    get_args(argc, argv);

    struct sigaction sa;
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);
    sigaction (SIGUSR1, &sa, NULL);    

    publish_pid_to_wd(DRONE_SYM, getpid());

    // Variable declaration
    int x; int y;
    int screen_size_x; int screen_size_y;
    int actionX; int actionY;
    double pos_x; double pos_y;

    // Initial values
    double Vx = 0.0; double Vy = 0.0;
    double forceX = 0.0; double forceY = 0.0;
    int obtained_obstacles = 0;

    // Targets and obstacles
    Obstacles obstacles[80];
    int numObstacles;

    while (1)
    {
        //////////////////////////////////////////////////////
        /* SECTION 1: READ DATA FROM SERVER */
        /////////////////////////////////////////////////////

        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_drone[0], &read_fds);
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
        
        char server_msg[MSG_LEN];

        int ready = select(server_drone[0] + 1, &read_fds, NULL, NULL, &timeout);
        if (ready == -1) {perror("Error in select");}

        if (ready > 0 && FD_ISSET(server_drone[0], &read_fds)) {
            ssize_t bytes_read = read(server_drone[0], server_msg, MSG_LEN);
            if (bytes_read > 0){
                // Message origin: Keyboard Manager
                if (server_msg[0] == 'K'){
                    sscanf(server_msg, "K:%d,%d", &actionX, &actionY);
                }
                // Message origin: Interface
                else if (server_msg[0] == 'I' && server_msg[1] == '1'){
                    sscanf(server_msg, "I1:%d,%d,%d,%d", &x, &y, &screen_size_x, &screen_size_y);
                    printf("Obtained initial parameters from server: %s\n", server_msg);
                    pos_x = (double)x;
                    pos_y = (double)y;
                    fflush(stdout);
                }
                // Message origin: Interface
                else if (server_msg[0] == 'I' && server_msg[1] == '2'){
                    sscanf(server_msg, "I2:%d,%d", &screen_size_x, &screen_size_y);
                    printf("Changed screen dimensions to: %s\n", server_msg);
                    fflush(stdout);
                }
                // Message origin: Obstacles
                else if (server_msg[0] == 'O'){
                    printf("Obtained obstacles message: %s\n", server_msg);
                    parseObstaclesMsg(server_msg, obstacles, &numObstacles);
                    obtained_obstacles = 1;
                }
            }
        } else { actionX = 0; actionY = 0;}

        char obstacles_msg[] = "O[1]200,200"; // Random for initial execution
        if (obtained_obstacles == 0){
            parseObstaclesMsg(obstacles_msg, obstacles, &numObstacles);
        }
        
        //////////////////////////////////////////////////////
        /* SECTION 2: OBTAIN THE FORCES EXERTED ON THE DRONE*/
        /////////////////////////////////////////////////////  

        /* INTERNAL FORCE from the drone itself */
        if (abs(actionX) <= 1.0){ // Only values between -1 to 1 are used to move the drone
            forceX += (double)actionX;
            forceY += (double)actionY;
            /* Capping to the max value of the drone's force */
            if (forceX > F_MAX){forceX = F_MAX;}
            if (forceX < -F_MAX){forceX = -F_MAX;}
            if (forceY > F_MAX){forceY = F_MAX;}
            if (forceY < -F_MAX){forceY = -F_MAX;}
        }
        else{forceX = 0.0,forceY = 0.0;} // Other values represent STOP

        /* EXTERNAL FORCE from obstacles and targets */
        double ext_forceX = 0.0; 
        double ext_forceY = 0.0;

        // TARGETS
        // TODO: Obtain the string for targets_msg from a pipe from (interface.c)
        char targets_msg[] = "140,23";
        double target_x, target_y;
        sscanf(targets_msg, "%lf,%lf", &target_x, &target_y);
        calculateExtForce(pos_x, pos_y, target_x, target_y, 0.0, 0.0, &ext_forceX, &ext_forceY);


        for (int i = 0; i < numObstacles; i++) {
            calculateExtForce(pos_x, pos_y, 0.0, 0.0, obstacles[i].x, obstacles[i].y, &ext_forceX, &ext_forceY);
        }
        
        //////////////////////////////////////////////////////
        /* SECTION 3: CALCULATE POSITION DATA */
        ///////////////////////////////////////////////////// 

        double max_x = (double)screen_size_x;
        double max_y = (double)screen_size_y;
        eulerMethod(&pos_x, &Vx, forceX, ext_forceX, &max_x);
        eulerMethod(&pos_y, &Vy, forceY, ext_forceY, &max_y);

        // Only print the data ONLY when there is movement from the drone.
        if (fabs(Vx) > FLOAT_TOLERANCE || fabs(Vy) > FLOAT_TOLERANCE)
        {
            printf("Drone force (X,Y): %.2f,%.2f\t|\t",forceX,forceY);
            printf("External force (X,Y): %.2f,%.2f\n",ext_forceX,ext_forceY);
            printf("X - Position: %.2f / Velocity: %.2f\t|\t", pos_x, Vx);
            printf("Y - Position: %.2f / Velocity: %.2f\n", pos_y, Vy);
            fflush(stdout);
        }

        int xf = (int)round(pos_x);
        int yf = (int)round(pos_y);

        //////////////////////////////////////////////////////
        /* SECTION 4: SEND THE DRONE POSITION TO SERVER */
        ///////////////////////////////////////////////////// 

        char position_msg[MSG_LEN];
        sprintf(position_msg, "D:%d,%d", xf, yf);
        write_to_pipe(drone_server[1], position_msg);

        usleep(D_T * 1e6); // 0.1 s
    }

    return 0;
}


void signal_handler(int signo, siginfo_t *siginfo, void *context) 
{
    //printf("Received signal number: %d \n", signo);
    if( signo == SIGINT)
    {
        printf("Caught SIGINT \n");
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

void get_args(int argc, char *argv[])
{
    sscanf(argv[1], "%d %d", &server_drone[0], &drone_server[1]);
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
    // if(*ext_forceX > 50.0){*ext_forceX=0;}
    // if(*ext_forceY > 50.0){*ext_forceY=0;}
    // if(*ext_forceX < 50.0){*ext_forceX=0;}
    // if(*ext_forceY < 50.0){*ext_forceY=0;}
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