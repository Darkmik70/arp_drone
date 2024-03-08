#include "drone.h"
#include "common.h"

// PIPES: File descriptor arrays
int interface_drone[2];  // Serverless pipe
int server_drone[2];
int drone_server[2];

char logfile[80]; // path to logfile
char msg[1024];


int main(int argc, char *argv[]) 
{
    // Read the file descriptors from the arguments
    get_args(argc, argv);

    // Signals
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
    int target_x; int target_y;
    int valid_target;
    Obstacles obstacles[80];
    int numObstacles;

    // Initial values
    double Vx = 0.0; double Vy = 0.0;
    double forceX = 0.0; double forceY = 0.0;
    int obtained_obstacles = 0;


    while (1)
    {
        //////////////////////////////////////////////////////
        /* SECTION 1: READ DATA FROM SERVER */
        /////////////////////////////////////////////////////

        char server_msg[MSG_LEN];
        if (read_pipe_unblocked(server_drone[0], server_msg) == 1){
            // Message origin: Keyboard Manager
            if (server_msg[0] == 'K'){
                sscanf(server_msg, "K:%d,%d", &actionX, &actionY);
            }
            // Message origin: Interface
            else if (server_msg[0] == 'I' && server_msg[1] == '1'){
                sscanf(server_msg, "I1:%d,%d,%d,%d", &x, &y, &screen_size_x, &screen_size_y);
                sprintf(msg, "Obtained initial parameters from server: %s\n", server_msg);
                log_msg(logfile, DRONE, msg);
                pos_x = (double)x;
                pos_y = (double)y;
            }
            // Message origin: Interface
            else if (server_msg[0] == 'I' && server_msg[1] == '2'){
                sscanf(server_msg, "I2:%d,%d", &screen_size_x, &screen_size_y);
            }
            // Message origin: Obstacles
            else if (server_msg[0] == 'O'){
                parse_obstacles_string(server_msg, obstacles, &numObstacles);
                obtained_obstacles = 1;
            }
        }
        else { actionX = 0; actionY = 0;}

        // If obstacles has not been obtained, continue loop.
        if (obtained_obstacles == 0){
            continue;
        }
        
        //////////////////////////////////////////////////////
        /* SECTION 2: READ DATA FROM INTERFACE.C (SERVERLESS PIPE) */
        /////////////////////////////////////////////////////

        char interface_msg[MSG_LEN];
        if (read_pipe_unblocked(interface_drone[0], interface_msg) == 1){
            sscanf(interface_msg, "%d,%d", &target_x, &target_y);
            valid_target = 1;
        }

        //////////////////////////////////////////////////////
        /* SECTION 3: OBTAIN THE FORCES EXERTED ON THE DRONE*/
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
        if(valid_target == 1){
            calculate_external_forces(pos_x, pos_y, target_x, target_y, 0.0, 0.0, &ext_forceX, &ext_forceY);
        }
        // OBSTACLES
        for (int i = 0; i < numObstacles; i++) {
            calculate_external_forces(pos_x, pos_y, 0.0, 0.0, obstacles[i].x, obstacles[i].y, &ext_forceX, &ext_forceY);
        }

        if(ext_forceX > EXT_FORCE_MAX){ext_forceX = EXT_FORCE_MAX;}
        if(ext_forceX < -EXT_FORCE_MAX){ext_forceX = -EXT_FORCE_MAX;}
        if(ext_forceY > EXT_FORCE_MAX){ext_forceY = EXT_FORCE_MAX;}
        if(ext_forceY < -EXT_FORCE_MAX){ext_forceY = -EXT_FORCE_MAX;}
        
        //////////////////////////////////////////////////////
        /* SECTION 3: CALCULATE POSITION DATA */
        ///////////////////////////////////////////////////// 

        double max_x = (double)screen_size_x;
        double max_y = (double)screen_size_y;
        euler_method(&pos_x, &Vx, forceX, ext_forceX, &max_x);
        euler_method(&pos_y, &Vy, forceY, ext_forceY, &max_y);

        // Only print the data ONLY when there is movement from the drone.
        if (fabs(Vx) > TOLERANCE || fabs(Vy) > TOLERANCE)
        {
            sprintf(msg, "Drone force (X,Y): %.2f,%.2f\t|\t",forceX,forceY);
            log_msg(logfile, DRONE, msg);
            sprintf(msg, "External force (X,Y): %.2f,%.2f\n",ext_forceX,ext_forceY);
            log_msg(logfile, DRONE, msg);
            sprintf(msg, "X - Position: %.2f / Velocity: %.2f\t|\t", pos_x, Vx);
            log_msg(logfile, DRONE, msg);
            sprintf(msg, "Y - Position: %.2f / Velocity: %.2f\n", pos_y, Vy);
            log_msg(logfile, DRONE, msg);
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
        sprintf(msg, "Caught SIGINT \n");
        log_msg(logfile, DRONE, msg);
        close(interface_drone[0]);
        close(interface_drone[1]);
        close(server_drone[0]);
        close(server_drone[1]);
        close(drone_server[0]);
        close(drone_server[1]);
        sprintf(msg,"Succesfully performed the cleanup\n");
        log_msg(logfile, DRONE, msg);
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


void calculate_external_forces(double droneX, double droneY, double targetX, double targetY,
                        double obstacleX, double obstacleY, double *ext_forceX, double *ext_forceY)
{
    // ***Calculate ATTRACTION force towards the target***
    double distanceToTarget = sqrt(pow(targetX - droneX, 2) + pow(targetY - droneY, 2));
    double angleToTarget = atan2(targetY - droneY, targetX - droneX);

    // To avoid division by zero or extremely high forces
    if (distanceToTarget < MAX_DISTANCE) {
        distanceToTarget = MAX_DISTANCE;
    }
    // Bellow 5m the attraction force will be calculated.
    else if (distanceToTarget < MIN_DISTANCE){
    *ext_forceX += COEFFICIENT * (1.0 / distanceToTarget - 1.0 / 5.0) * (1.0 / pow(distanceToTarget,2)) * cos(angleToTarget);
    *ext_forceY += COEFFICIENT * (1.0 / distanceToTarget - 1.0 / 5.0) * (1.0 / pow(distanceToTarget,2)) * sin(angleToTarget);
    }
    else{
        *ext_forceX += 0;
        *ext_forceY += 0;  
    }

    // ***Calculate REPULSION force from the obstacle***
    double distanceToObstacle = sqrt(pow(obstacleX - droneX, 2) + pow(obstacleY - droneY, 2));
    double angleToObstacle = atan2(obstacleY - droneY, obstacleX - droneX);

    // To avoid division by zero or extremely high forces
    if (distanceToObstacle < MAX_DISTANCE) {
            distanceToObstacle = MAX_DISTANCE; // Keep the force value as it were on the MAX_DISTANCE
        }
    // Bellow 5m the repulsion force will be calculated
    else if (distanceToObstacle < MIN_DISTANCE){
    *ext_forceX -= COEFFICIENT * (1.0 / distanceToObstacle - 1.0 / 5.0) * (1.0 / pow(distanceToObstacle,2)) * cos(angleToObstacle);
    *ext_forceY -= COEFFICIENT * (1.0 / distanceToObstacle - 1.0 / 5.0) * (1.0 / pow(distanceToObstacle,2)) * sin(angleToObstacle);
    }
    else{
        *ext_forceX -= 0;
        *ext_forceY -= 0;
    }
}


void euler_method(double *pos, double *vel, double force, double extForce, double *maxPos)
{
    double total_force = force + extForce;
    double accelerationX = (total_force - DAMPING * (*vel)) / MASS;
    *vel = *vel + accelerationX * D_T;
    *pos = *pos + (*vel) * D_T;

    // Walls are the maximum position the drone can reach
    if (*pos < 0){*pos = 0;}
    if (*pos > *maxPos){*pos = *maxPos - 1;}
}


void parse_obstacles_string(char *obstacles_msg, Obstacles *obstacles, int *numObstacles)
{
    int totalObstacles;
    sscanf(obstacles_msg, "O[%d]", &totalObstacles);

    char *token = strtok(obstacles_msg + 4, "|");
    *numObstacles = 0;

    while (token != NULL && *numObstacles < totalObstacles) {
        float x_float, y_float;
        sscanf(token, "%f,%f", &x_float, &y_float);

        // Convert float to int (rounding is acceptable)
        obstacles[*numObstacles].x = (int)(x_float + 0.5);
        obstacles[*numObstacles].y = (int)(y_float + 0.5);

        obstacles[*numObstacles].total = *numObstacles + 1;

        token = strtok(NULL, "|");
        (*numObstacles)++;
    }
}


void get_args(int argc, char *argv[])
{
    sscanf(argv[1], "%d %d %d %s", &server_drone[0], &drone_server[1], &interface_drone[0], logfile);
}