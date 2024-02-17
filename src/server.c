#include "server.h"
#include "constants.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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


// Pipes
int km_server[2];
int server_drone[2];
int interface_server[2];
int drone_server[2];
int server_interface[2];
int server_obstacles[2];
int obstacles_server[2];
int server_targets[2];
int targets_server[2];

// Shared memory and semaphores
void *ptr_wd;                           // Shared memory for WD
void *ptr_logs;                         // Shared memory ptr for logs

sem_t *sem_logs_1;
sem_t *sem_logs_2;
sem_t *sem_logs_3;
sem_t *sem_wd_1, *sem_wd_2, *sem_wd_3;  // Semaphores for watchdog

void read_then_echo(int sockfd, char msg[]);
int read_then_echo_unblocked(int sockfd, char msg[]);
void write_then_wait_echo(int sockfd, char msg[], size_t);
int read_pipe_unblocked(int pipefd, char msg[]);

int main(int argc, char *argv[]) 
{   
    get_args(argc, argv);
    /* Sigaction */
    struct sigaction sa;
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO;
    sigaction (SIGINT, &sa, NULL);    
    sigaction (SIGUSR1, &sa, NULL);

    // Shared memory and semaphores for WATCHDOG
    ptr_wd = create_shm(SHM_WD);
    sem_wd_1 = sem_open(SEM_WD_1, O_CREAT, S_IRUSR | S_IWUSR, 0); // 0 for locked, this semaphore is unlocked by WD in order to get pids
    if (sem_wd_1 == SEM_FAILED) {
        perror("sem_wd_1 failed");
        exit(1);
    }
    sem_wd_2 = sem_open(SEM_WD_2, O_CREAT, S_IRUSR | S_IWUSR, 0); // 0 for locked, this semaphore is unlocked by WD in order to get pids
    if (sem_wd_2 == SEM_FAILED) {
        perror("sem_wd_2 failed");
        exit(1);
    }
    sem_wd_3 = sem_open(SEM_WD_3, O_CREAT, S_IRUSR | S_IWUSR, 0); // 0 for locked, this semaphore is unlocked by WD in order to get pids
    if (sem_wd_3 == SEM_FAILED) {
        perror("sem_wd_3 failed");
        exit(1);
    }

    // Shared memory for LOGS
    ptr_logs = create_shm(SHM_LOGS);
    sem_logs_1 = sem_open(SEM_LOGS_1, O_CREAT, S_IRUSR | S_IWUSR, 0); // this dude is locked
    if (sem_logs_1 == SEM_FAILED) {
        perror("sem_logs_1 failed");
        exit(1);
    }
    sem_logs_2 = sem_open(SEM_LOGS_2, O_CREAT, S_IRUSR | S_IWUSR, 0); 
    if (sem_logs_2 == SEM_FAILED) {
        perror("sem_logs_2 failed");
        exit(1);
    }
    sem_logs_3 = sem_open(SEM_LOGS_3, O_CREAT, S_IRUSR | S_IWUSR, 0); 
    if (sem_logs_3 == SEM_FAILED) {
        perror("sem_logs_3 failed");
        exit(1);
    }

    // When all shm are created publish your pid to WD
    publish_pid_to_wd(SERVER_SYM, getpid());

    // Timeout
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    // To compare current and previous data
    char prev_drone_msg[MSG_LEN] = "";

    //////////////////////////////////////////////////////
    /* SOCKET CREATION */
    /////////////////////////////////////////////////////

    int obstacles_sockfd = 0;
    int targets_sockfd = 0;
    int sockfd, newsockfd, portno, clilen, pid;
    struct sockaddr_in serv_addr, cli_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
    perror("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = PORT_NUMBER;  // Hard-coded port number
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
            perror("ERROR on binding");
    }
    listen(sockfd,5);
    clilen = sizeof(cli_addr);

    //////////////////////////////////////////////////////
    /* HANDLE CLIENT CONNECTIONS */
    /////////////////////////////////////////////////////
  
    while(targets_sockfd == 0 || obstacles_sockfd == 0){
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) {perror("ERROR on accept");}

        char socket_msg[MSG_LEN];
        read_then_echo(newsockfd, socket_msg);

        if (socket_msg[0] == 'O'){obstacles_sockfd = newsockfd;}
        else if(socket_msg[0] == 'T'){targets_sockfd = newsockfd;}
        else {close(newsockfd);}
    }

    while (1)
    {
        //////////////////////////////////////////////////////
        /* Handle pipe from key_manager.c */
        /////////////////////////////////////////////////////
        
        char km_msg[MSG_LEN];
        if (read_pipe_unblocked(km_server[0], km_msg) == 1){
                // Read acknowledgement
                printf("[PIPE] Received from key_manager.c: %s\n", km_msg);
                fflush(stdout);
                // Response
                char response_km_msg[MSG_LEN*2];
                sprintf(response_km_msg, "K:%s", km_msg);
                write_to_pipe(server_drone[1], response_km_msg);
                printf("[PIPE] Sent to drone.c: %s\n", response_km_msg);
                fflush(stdout);
        }
        
        //////////////////////////////////////////////////////
        /* Handle pipe from interface.c */
        /////////////////////////////////////////////////////
        
        char interface_msg[MSG_LEN];
        memset(interface_msg, 0 , MSG_LEN);

        if (read_pipe_unblocked(interface_server[0], interface_msg) == 1){
            // Read acknowledgement
            printf("[PIPE] Received from interface.c: %s\n", interface_msg);
            fflush(stdout);
            // Response for drone.c
            write_to_pipe(server_drone[1], interface_msg);
            printf("[PIPE] Sent to drone.c: %s\n", interface_msg);
            fflush(stdout);

            if(interface_msg[0] == 'I' && interface_msg[1] == '2'){
                // Send to socket: Client Targets
                char substring1[MSG_LEN];
                strcpy(substring1, interface_msg +3);
                write_then_wait_echo(targets_sockfd, substring1, sizeof(substring1));
                // Send to socket: Client Obstacles
                char substring2[MSG_LEN];
                strcpy(substring2, interface_msg +3);
                write_then_wait_echo(obstacles_sockfd, substring2, sizeof(substring2));
                printf("SENT %s to obstacles.c and targets.c\n", substring2);
            }
        }    
        

        //////////////////////////////////////////////////////
        /* Handle pipe from drone.c */
        /////////////////////////////////////////////////////
        
        char drone_msg[MSG_LEN];
        if (read_pipe_unblocked(drone_server[0], drone_msg) == 1){
            write_to_pipe(server_interface[1], drone_msg);
            // Only send to drone when data has changed
            if (strcmp(drone_msg, prev_drone_msg) != 0) {
                printf("[PIPE] Received from drone.c: %s\n", drone_msg);
                fflush(stdout);
                // Update previous values
                strcpy(prev_drone_msg, drone_msg); 
                printf("[PIPE] Sent to interface.c: %s\n", drone_msg);
                fflush(stdout);
            }
        }

        //////////////////////////////////////////////////////
        /* Handle socket from obstacles.c */
        /////////////////////////////////////////////////////

        char obstacles_msg[MSG_LEN];
        if (read_then_echo_unblocked(obstacles_sockfd, obstacles_msg) == 1){
            // Send to interface.c
            write_to_pipe(server_interface[1],obstacles_msg);
            printf("[PIPE] Send to interface.c: %s\n", obstacles_msg);
            fflush(stdout);
            // Send to drone.c
            write_to_pipe(server_drone[1], obstacles_msg);
            printf("[PIPE] Sent to drone.c: %s\n", obstacles_msg);
            fflush(stdout);
        }

        //////////////////////////////////////////////////////
        /* Handle socket from targets.c */
        /////////////////////////////////////////////////////

        char targets_msg[MSG_LEN];
        if (read_then_echo_unblocked(targets_sockfd, targets_msg) == 1){
            // Send to interface.c
            write_to_pipe(server_interface[1],targets_msg);
            printf("[PIPE] Sent to interface.c: %s\n", targets_msg);
            fflush(stdout);
        }

    }

    // Close and unlink the semaphores
    clean_up();
    return 0;
}


void signal_handler(int signo, siginfo_t *siginfo, void *context) 
{
    // printf("Received signal number: %d \n", signo);
    if (signo == SIGINT)
    {
        printf("Caught SIGINT\n");
        clean_up();
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


void *create_shm(char *name)
{
    int shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("shm_open of shm_key");
        exit(1);
    }
    /* configure the size of the shared memory object */
    ftruncate(shm_fd, SIZE_SHM);

    /* memory map the shared memory segment */
    void *shm_ptr = mmap(0, SIZE_SHM, PROT_WRITE | PROT_READ, MAP_SHARED, shm_fd, 0); 
    if (shm_ptr == MAP_FAILED)
    {
        perror("Map Failed");
        exit(1);
    }
    // File descriptor is no longer needed
    close(shm_fd);

    return shm_ptr;
}


void clean_up()
{
    // Close all connections
    sem_close(sem_wd_1);
    sem_close(sem_wd_2);
    sem_close(sem_wd_3);
    // Unlink semaphores
    sem_unlink(SEM_WD_1);
    sem_unlink(SEM_WD_2);
    sem_unlink(SEM_WD_3);
    // Unmap shared memory
    munmap(ptr_wd, SIZE_SHM);
    // Unlink shared memories
    shm_unlink(SHM_WD);
    printf("Clean up has been performed succesfully\n");
}


void get_args(int argc, char *argv[])
{
    sscanf(argv[1], "%d %d %d %d %d %d %d %d %d", &km_server[0], &server_drone[1], 
    &interface_server[0], &drone_server[0], &server_interface[1],
    &server_obstacles[1], &obstacles_server[0], &server_targets[1],
    &targets_server[0]);
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


int read_pipe_unblocked(int pipefd, char msg[]){
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    fd_set read_pipe;
    FD_ZERO(&read_pipe);
    FD_SET(pipefd, &read_pipe);

    char buffer[MSG_LEN];

    int ready_km = select(pipefd + 1, &read_pipe, NULL, NULL, &timeout);
    if (ready_km == -1) {perror("Error in select");}

    if (ready_km > 0 && FD_ISSET(pipefd, &read_pipe)) {
        ssize_t bytes_read_pipe = read(pipefd, buffer, MSG_LEN);
        if (bytes_read_pipe > 0) {
            strcpy(msg, buffer);
            return 1;
        }
        else{return 0;}
    }
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

