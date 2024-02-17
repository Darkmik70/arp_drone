#include "obstacles.h"
#include "util.h"
#include "constants.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sys/time.h>

#include <semaphore.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

// Pipes working with the server
int server_obstacles[2];
int obstacles_server[2];

void read_then_echo(int sockfd, char msg[]);
int read_then_echo_unblocked(int sockfd, char msg[]);
void write_then_wait_echo(int sockfd, char msg[], size_t);


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

    portno = PORT_NUMBER;  // Hard-coded port number
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

    char init_msg[MSG_LEN];
    strcpy(init_msg, "OI");
    write_then_wait_echo(sockfd, init_msg, sizeof(init_msg));

    //////////////////////////////////////////////////////
    /* OBTAIN DIMENSIONS */
    /////////////////////////////////////////////////////

    // According to protocol, next data from server will be the screen dimensions
    float temp_scx, temp_scy;
    char dimension_msg[MSG_LEN];
    read_then_echo(sockfd, dimension_msg);

    sscanf(dimension_msg, "%f,%f", &temp_scx, &temp_scy);
    screen_size_x = (int)temp_scx;
    screen_size_y = (int)temp_scy;

    while (1) {

        //////////////////////////////////////////////////////
        /* SECTION 1: READ THE DATA FROM SERVER*/
        /////////////////////////////////////////////////////

        char socket_msg[MSG_LEN];   
        read_then_echo_unblocked(sockfd, socket_msg);

        if (strcmp(socket_msg, "STOP") == 0){
            printf("STOP RECEIVED FROM SERVER!\n");
            fflush(stdout);
            printf("This process will close in 5 seconds...");
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


void read_then_echo(int sockfd, char socket_msg[]){
    int bytes_read, bytes_written;
    bzero(socket_msg, MSG_LEN);

    // Read from the socket
    bytes_read = read(sockfd, socket_msg, MSG_LEN - 1);
    if (bytes_read < 0) perror("ERROR reading from socket");
    printf("[SOCKET] Received: %s\n", socket_msg);
    
    // Echo data read into socket
    bytes_written = write(sockfd, socket_msg, bytes_read);
    printf("[SOCKET] Echo sent: %s\n", socket_msg);
}


int read_then_echo_unblocked(int sockfd, char socket_msg[]) {
    int ready;
    int bytes_read, bytes_written;
    fd_set read_fds;
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    // Clear the buffer
    bzero(socket_msg, MSG_LEN);

    // Initialize the set of file descriptors to monitor for reading
    FD_ZERO(&read_fds);
    FD_SET(sockfd, &read_fds);

    // Use select to check if the socket is ready for reading
    ready = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);
    if (ready < 0) {perror("ERROR in select");} 
    else if (ready == 0) {return 0;}  // No data available

    // Data is available for reading, so read from the socket
    bytes_read = read(sockfd, socket_msg, MSG_LEN - 1);
    if (bytes_read < 0) {perror("ERROR reading from socket");} 
    else if (bytes_read == 0) {return 0;}  // Connection closed
    else if (socket_msg[0] == '\0') {return 0;} // Empty string

    // Print the received message
    printf("[SOCKET] Received: %s\n", socket_msg);

    // Echo the message back to the client
    bytes_written = write(sockfd, socket_msg, bytes_read);
    if (bytes_written < 0) {perror("ERROR writing to socket");}
    else{printf("[SOCKET] Echo sent: %s\n", socket_msg); return 1;}
}


void write_then_wait_echo(int sockfd, char socket_msg[], size_t msg_size){
    int ready;
    int bytes_read, bytes_written;

    bytes_written = write(sockfd, socket_msg, msg_size);
    if (bytes_written < 0) {perror("ERROR writing to socket");}
    printf("[SOCKET] Sent: %s\n", socket_msg);

    // Clear the buffer
    bzero(socket_msg, MSG_LEN);

    while (socket_msg[0] == '\0'){
        // Data is available for reading, so read from the socket
        bytes_read = read(sockfd, socket_msg, bytes_written);
        if (bytes_read < 0) {perror("ERROR reading from socket");} 
        else if (bytes_read == 0) {printf("Connection closed!\n"); return;}
    }
    // Print the received message
    printf("[SOCKET] Echo received: %s\n", socket_msg);
}


