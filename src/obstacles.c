#include "obstacles.h"
#include "common.h"

// Pipes working with the server
int server_obstacles[2];
int obstacles_server[2];

char logfile[80]; // path to logfile
char msg[1024];


int main(int argc, char *argv[])
{   
    // Read the file descriptors from the arguments
    get_args(argc, argv);

    sleep(1);
    // Read the config.txt file
    char program_type[MSG_LEN];
    char socket_data[MSG_LEN];
    read_args_from_file(CONFIG_PATH, program_type, socket_data);
    char host_name[MSG_LEN];
    int port_number;
    sprintf(msg, "Program type: %s\n", program_type);
    log_msg(logfile, OBS, msg);

    parse_host_port(socket_data, host_name, &port_number);
    sprintf(msg, "Host name: %s\n", host_name);
    log_msg(logfile, OBS, msg);

    sprintf(msg, "Port number: %d\n", port_number);
    log_msg(logfile, OBS, msg);

    if (strcmp(program_type, "server") == 0)
    {
        sprintf(msg, "Server mode - exiting process");
        log_msg(logfile, OBS, msg);
        exit(0);
    }

    // Signals
    struct sigaction sa;
    sa.sa_sigaction = signal_handler; 
    sa.sa_flags = SA_SIGINFO;
    sigaction (SIGINT, &sa, NULL);
    sigaction (SIGUSR1, &sa, NULL);   
    publish_pid_to_wd(OBSTACLES_SYM, getpid());

    // Seed random number generator with current time
    srand(time(NULL));

    // Variables
    Obstacle obstacles[MAX_OBSTACLES];
    int numObstacles = 0;
    char obstacles_msg[MSG_LEN];
    int screen_size_x; int screen_size_y;

    //////////////////////////////////////////////////////
    /* SOCKET INITIALIZATION */
    /////////////////////////////////////////////////////

    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    portno = port_number;  // Obtained from config.txt

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {log_err(logfile, OBS, "ERROR opening socket");}

    server = gethostbyname(host_name);  // Obtained from config.txt
    if (server == NULL) {fprintf(stderr,"ERROR, no such host\n");}

    bzero((char *) &serv_addr, sizeof(serv_addr)); 
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);

    block_signal(SIGUSR1);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        log_err(logfile, OBS, "ERROR connecting");
    unblock_signal(SIGUSR1);
    //////////////////////////////////////////////////////
    /* IDENTIFICATION WITH SERVER */
    /////////////////////////////////////////////////////

    char init_msg[1024] = "OI";
    write_then_wait_echo(sockfd, init_msg, sizeof(init_msg),logfile, OBS);

    //////////////////////////////////////////////////////
    /* OBTAIN DIMENSIONS */
    /////////////////////////////////////////////////////

    // According to protocol, this will be the screen dimensions.
    char dimension_msg[MSG_LEN];
    read_then_echo(sockfd, dimension_msg);
    log_msg(logfile, "OBSTACLES - SOCKET", dimension_msg);


    float temp_scx, temp_scy;
    sscanf(dimension_msg, "%f,%f", &temp_scx, &temp_scy);
    screen_size_x = (int)temp_scx;
    screen_size_y = (int)temp_scy;
    
    sleep(1);

    while (1) {

        //////////////////////////////////////////////////////
        /* SECTION 1: READ THE DATA FROM SERVER */
        /////////////////////////////////////////////////////

        char socket_msg[MSG_LEN];   

        read_then_echo_unblocked(sockfd, socket_msg, logfile, OBS);

        if (strcmp(socket_msg, "STOP") == 0){
            sprintf(msg,"STOP RECEIVED FROM SERVER!");
            log_msg(logfile, OBS, msg);
            sprintf(msg,"This process will close in 5 seconds...");
            log_msg(logfile, OBS, msg);
            fflush(stdout);
            sleep(5);
            exit(0);
        }

        //////////////////////////////////////////////////////
        /* SECTION 2: OBSTACLES GENERATION & SEND DATA */
        /////////////////////////////////////////////////////

        // Check if it's time to spawn a new obstacle
        if (numObstacles < MAX_OBSTACLES) {
            Obstacle newObstacle;
            newObstacle.x = rand() % screen_size_x;
            newObstacle.y = rand() % screen_size_y;
            newObstacle.spawnTime = time(NULL) + (rand() % (MAX_SPAWN_TIME - MIN_SPAWN_TIME + 1) + MIN_SPAWN_TIME);
            obstacles[numObstacles] = newObstacle;
            numObstacles++;
        }

        // Print obstacles every (WAIT_TIME)
        obstacles_msg[0] = '\0'; // Clear the string
        generate_obstacles_string(obstacles, numObstacles, obstacles_msg); 
        sleep(WAIT_TIME);

        // Check if any obstacle has exceeded its spawn time
        time_t currentTime = time(NULL);
        for (int i = 0; i < numObstacles; ++i) {
            if (obstacles[i].spawnTime <= currentTime) {
                // Replace the obstacle with a new one
                obstacles[i].x = rand() % screen_size_x;
                obstacles[i].y = rand() % screen_size_y;
                obstacles[i].spawnTime = time(NULL) + (rand() % (MAX_SPAWN_TIME - MIN_SPAWN_TIME + 1) + MIN_SPAWN_TIME);
            }
        }

        // SEND DATA TO SERVER
        write_then_wait_echo(sockfd, obstacles_msg, sizeof(obstacles_msg), logfile, "OBSTSACLES");
    }

    return 0;
}


void generate_obstacles_string(Obstacle obstacles[], int numObstacles, char obstacles_msg[])
{
    // Append the letter O and the total number of obstacles to obstacles_msg
    sprintf(obstacles_msg, "O[%d]", numObstacles);

    for (int i = 0; i < numObstacles; ++i) {
        // Append obstacle information to obstacles_msg
        sprintf(obstacles_msg + strlen(obstacles_msg), "%.3f,%.3f", 
        (float)obstacles[i].x, (float)obstacles[i].y);

        // Add a separator if there are more obstacles
        if (i < numObstacles - 1) {
            sprintf(obstacles_msg + strlen(obstacles_msg), "|");
        }
    }
    sprintf(msg, "Generated Obstacles - %s", obstacles_msg);
    log_msg(logfile, OBS, msg);

    fflush(stdout);
}

void signal_handler(int signo, siginfo_t *siginfo, void *context) 
{
    // printf("Received signal number: %d \n", signo);
    if  (signo == SIGINT)
    {
        sprintf(msg, "Caught SIGINT");
        log_msg(logfile, OBS, msg);
        close(obstacles_server[1]);
        close(server_obstacles[0]);
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
    sscanf(argv[1], "%d %d %s", &server_obstacles[0], &obstacles_server[1], logfile);
}

