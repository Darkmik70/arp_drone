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

char* read_then_echo(int sock);
char* read_then_echo_unblocked(int sock);
void write_then_wait_echo(int sock, char *msg);

void error(char *msg)
{
    perror(msg);
    //exit(0);
}

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
    int obtained_dimensions = 0;
    int screen_size_x; int screen_size_y;

    //////////////////////////////////////////////////////
    /* SOCKET INITIALIZATION */
    /////////////////////////////////////////////////////

    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    portno = 2000;  // Hard-coded port number
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {error("ERROR opening socket");}
    server = gethostbyname("localhost");
    if (server == NULL) {fprintf(stderr,"ERROR, no such host\n");}

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");

    //////////////////////////////////////////////////////
    /* IDENTIFICATION WITH SERVER */
    /////////////////////////////////////////////////////

    char msg[MSG_LEN];
    strcpy(msg, "OI");
    write_then_wait_echo(sockfd, msg);

    while (1) {

        //////////////////////////////////////////////////////
        /* SECTION 1: READ THE DATA FROM SERVER*/
        /////////////////////////////////////////////////////

        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_obstacles[0], &read_fds);
        
        char server_msg[MSG_LEN];

        int ready = select(server_obstacles[0] + 1, &read_fds, NULL, NULL, &timeout);
        if (ready == -1) {perror("Error in select");}

        if (ready > 0 && FD_ISSET(server_obstacles[0], &read_fds)) {
            ssize_t bytes_read = read(server_obstacles[0], server_msg, MSG_LEN);
            if (bytes_read > 0) {
                float temp_scx, temp_scy;
                sscanf(server_msg, "I2:%f,%f", &temp_scx, &temp_scy);
                screen_size_x = (int)temp_scx;
                screen_size_y = (int)temp_scy;
                printf("Obtained from server: %s\n", server_msg);
                fflush(stdout);
                obtained_dimensions = 1;
            }
        }
        if(obtained_dimensions == 0){continue;}


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
        write_to_pipe(obstacles_server[1],obstacles_msg);

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


char* read_then_echo(int sock){
    int n, n2;
    static char buffer[MSG_LEN];
    bzero(buffer, MSG_LEN);

    // Read from the socket
    n = read(sock, buffer, MSG_LEN - 1);
    if (n < 0) error("ERROR reading from socket");
    printf("Message obtained: %s\n", buffer);
    
    // Echo data read into socket
    n2 = write(sock, buffer, n);
    return buffer;
}


char* read_then_echo_unblocked(int sock) {
    static char buffer[MSG_LEN];
    fd_set read_fds;
    struct timeval timeout;
    int n, n2, ready;

    // Clear the buffer
    bzero(buffer, MSG_LEN);

    // Initialize the set of file descriptors to monitor for reading
    FD_ZERO(&read_fds);
    FD_SET(sock, &read_fds);

    // Set the timeout for select (0 seconds, 0 microseconds)
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    // Use select to check if the socket is ready for reading
    ready = select(sock + 1, &read_fds, NULL, NULL, &timeout);
    if (ready < 0) {perror("ERROR in select");} 
    else if (ready == 0) {return NULL;}  // No data available

    // Data is available for reading, so read from the socket
    n = read(sock, buffer, MSG_LEN - 1);
    if (n < 0) {perror("ERROR reading from socket");} 
    else if (n == 0) {return NULL;}  // Connection closed

    // Print the received message
    printf("Message obtained: %s\n", buffer);

    // Echo the message back to the client
    n2 = write(sock, buffer, n);
    if (n < 0) {perror("ERROR writing to socket");}

    return buffer;
}


void write_then_wait_echo(int sock, char *msg){
    static char buffer[MSG_LEN];
    fd_set read_fds;
    struct timeval timeout;
    int n, n2, ready;

    n2 = write(sock, msg, sizeof(msg));
    if (n < 0) {perror("ERROR writing to socket");}
    printf("Data sent to server: %s\n", msg);

    // Clear the buffer
    bzero(buffer, MSG_LEN);

    // Initialize the set of file descriptors to monitor for reading
    FD_ZERO(&read_fds);
    FD_SET(sock, &read_fds);

    // Set the timeout for select (0 seconds, 0 microseconds)
    timeout.tv_sec = 0;
    timeout.tv_usec = 300000;

    // Use select to check if the socket is ready for reading
    ready = select(sock + 1, &read_fds, NULL, NULL, &timeout);
    if (ready < 0) {perror("ERROR in select");} 
    else if (ready == 0) {
        printf("Timeout from server. Connection lost!");
        return;
    }  // No data available

    // Data is available for reading, so read from the socket
    n = read(sock, buffer, MSG_LEN - 1);
    if (n < 0) {perror("ERROR reading from socket");} 
    else if (n == 0) {
        printf("Connection closed!");
        return;
        }  // Connection closed

    // Print the received message
    printf("Response from server: %s\n", buffer);
}

