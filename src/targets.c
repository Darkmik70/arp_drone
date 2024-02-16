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

char* read_then_echo(int sock);
char* read_then_echo_unblocked(int sock);
void write_then_wait_echo(int sock, char *msg);

void error(char *msg)
{
    perror(msg);
    //exit(0);
}

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
    strcpy(msg, "TI");
    write_then_wait_echo(sockfd, msg);

    while(1)
    {
        //////////////////////////////////////////////////////
        /* SECTION 1: READ THE DATA FROM SERVER*/
        /////////////////////////////////////////////////////

        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_targets[0], &read_fds);
        
        char server_msg[MSG_LEN];

        int ready = select(server_targets[0] + 1, &read_fds, NULL, NULL, &timeout);
        if (ready == -1) {perror("Error in select");}

        if (ready > 0 && FD_ISSET(server_targets[0], &read_fds)) {
            ssize_t bytes_read = read(server_targets[0], server_msg, MSG_LEN);
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
        /* SECTION 2: TARGET GENERATION & SEND DATA*/
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
            printf("%s\n", targets_msg);
            write_to_pipe(targets_server[1],targets_msg);
            targets_created = 1;
        }

        // Delay
        usleep(500000);  
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


