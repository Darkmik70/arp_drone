#include "obstacles.h"
#include "common.h"

// Pipes working with the server
int server_obstacles[2];
int obstacles_server[2];


int main(int argc, char *argv[])
{   
    sleep(1);
    get_args(argc, argv);
    srand(time(NULL));

    // Timeout
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

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

    portno = PORT_NUMBER;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {perror("ERROR opening socket");}

    server = gethostbyname("localhost");
    if (server == NULL) {fprintf(stderr,"ERROR, no such host\n");}

    bzero((char *) &serv_addr, sizeof(serv_addr)); 
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);

    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
        perror("ERROR connecting");

    //////////////////////////////////////////////////////
    /* IDENTIFICATION WITH SERVER */
    /////////////////////////////////////////////////////

    char init_msg[] = "OI";
    write_then_wait_echo(sockfd, init_msg, sizeof(init_msg));

    //////////////////////////////////////////////////////
    /* OBTAIN DIMENSIONS */
    /////////////////////////////////////////////////////

    // According to protocol, this will be the screen dimensions.
    char dimension_msg[MSG_LEN];
    read_then_echo(sockfd, dimension_msg);

    float temp_scx, temp_scy;
    sscanf(dimension_msg, "%f,%f", &temp_scx, &temp_scy);
    screen_size_x = (int)temp_scx;
    screen_size_y = (int)temp_scy;

    while (1) {

        //////////////////////////////////////////////////////
        /* SECTION 1: READ THE DATA FROM SERVER */
        /////////////////////////////////////////////////////

        char socket_msg[MSG_LEN];   
        read_then_echo_unblocked(sockfd, socket_msg);

        if (strcmp(socket_msg, "STOP") == 0){
            printf("STOP RECEIVED FROM SERVER!\n");
            printf("This process will close in 5 seconds...");
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
        printObstacles(obstacles, numObstacles, obstacles_msg); 
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
        write_then_wait_echo(sockfd, obstacles_msg, sizeof(obstacles_msg));
    }

    return 0;
}


void printObstacles(Obstacle obstacles[], int numObstacles, char obstacles_msg[])
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
    printf("%s\n", obstacles_msg);
    fflush(stdout);
}


void get_args(int argc, char *argv[])
{
    sscanf(argv[1], "%d %d", &server_obstacles[0], &obstacles_server[1]);
}

