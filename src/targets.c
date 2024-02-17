#include "targets.h"
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
int server_targets[0];
int targets_server[1];

void read_then_echo(int sockfd, char msg[]);
int read_then_echo_unblocked(int sockfd, char msg[]);
void write_then_wait_echo(int sockfd, char msg[], size_t);


int main(int argc, char *argv[]) {
    sleep(1);
    get_args(argc, argv);
    srand(time(NULL));

    // Variables
    int screen_size_x; int screen_size_y;
    float scale_x;
    float scale_y;
    int counter = 0;
    int obtained_dimensions = 0;
    int targets_created = 0;
    
    // Timeout
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

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
    strcpy(init_msg, "TI");
    write_then_wait_echo(sockfd, init_msg, sizeof(init_msg));

    //////////////////////////////////////////////////////
    /* OBTAIN DIMENSIONS */
    /////////////////////////////////////////////////////

    // According to protocol, next data from server will be the screen dimensions
    char dimension_msg[MSG_LEN];
    read_then_echo(sockfd, dimension_msg);
    float temp_scx, temp_scy;
    sscanf(dimension_msg, "%f,%f", &temp_scx, &temp_scy);
    screen_size_x = (int)temp_scx;
    screen_size_y = (int)temp_scy;

    while(1)
    {
        //////////////////////////////////////////////////////
        /* SECTION 1: TARGET GENERATION & SEND DATA*/
        /////////////////////////////////////////////////////
        
        // Array to store generated targets
        Target targets[MAX_TARGETS];
        // Set seed for random number generation
        srand(time(NULL));

        if(targets_created == 0){
            // Sector dimensions
            const int sectorWidth = screen_size_x / 3;
            const int sectorHeight = screen_size_y / 2;

            // Generate random order for distributing targets across sectors
            int order[MAX_TARGETS];
            for (int i = 0; i < MAX_TARGETS; ++i) {
                order[i] = i;
            }
            for (int i = MAX_TARGETS - 1; i > 0; --i) {
                int j = rand() % (i + 1);
                int temp = order[i];
                order[i] = order[j];
                order[j] = temp;
            }

            // String variable to store the targets in the specified format
            char targets_msg[MSG_LEN]; // Adjust the size accordingly

            // Generate targets within each sector
            for (int i = 0; i < MAX_TARGETS; ++i) {
                Target target;

                // Determine sector based on the random order
                int sectorX = (order[i] % 3) * sectorWidth;
                int sectorY = (order[i] / 3) * sectorHeight;

                // Generate random coordinates within the sector
                generateRandomCoordinates(sectorWidth, sectorHeight, &target.x, &target.y);

                // Adjust coordinates based on the sector position
                target.x += sectorX;
                target.y += sectorY;

                // Ensure coordinates do not exceed the screen size
                target.x = target.x % screen_size_x;
                target.y = target.y % screen_size_y;

                // Store the target
                targets[i] = target;
            }

            // Construct the targets_msg string
            int offset = sprintf(targets_msg, "T[%d]", MAX_TARGETS);
            for (int i = 0; i < MAX_TARGETS; ++i) {
                offset += sprintf(targets_msg + offset, "%.3f,%.3f", 
                                    (float)targets[i].x, (float)targets[i].y);

                // Add a separator unless it's the last element
                if (i < MAX_TARGETS - 1) {
                    offset += sprintf(targets_msg + offset, "|");
                }
            }
            targets_msg[offset] = '\0'; // Null-terminate the string

            // Send the data to the server
            write_then_wait_echo(sockfd, targets_msg, sizeof(targets_msg));
            targets_created = 1;
        }

        //////////////////////////////////////////////////////
        /* SECTION 2: READ THE DATA FROM SERVER*/
        /////////////////////////////////////////////////////

        char socket_msg[MSG_LEN];
        // We use blocking read because targets do not not continous operation
        read_then_echo_unblocked(sockfd, socket_msg);

        if (strcmp(socket_msg, "STOP") == 0){
            printf("STOP RECEIVED FROM SERVER!\n");
            fflush(stdout);
            printf("This process will close in 5 seconds...\n");
            sleep(5);
            exit(0);
        }
        else if (strcmp(socket_msg, "GE") == 0){
            printf("GE RECEIVED FROM SERVER!\n");
            fflush(stdout);
            printf("Regenerating targets...");
        }
        usleep(200000);
    }

    return 0;
}


// Function to generate random coordinates within a given sector
void generateRandomCoordinates(int sectorWidth, int sectorHeight, int *x, int *y)
{
    *x = rand() % sectorWidth;
    *y = rand() % sectorHeight;
}


void get_args(int argc, char *argv[])
{
    sscanf(argv[1], "%d %d", &server_targets[0], &targets_server[1]);
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

