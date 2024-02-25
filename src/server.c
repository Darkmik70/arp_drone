#include "server.h"
#include "common.h"

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
void *ptr_wd;                                   // Shared memory for WD
void *ptr_logs;                                 // Shared memory for logger
sem_t *sem_wd_1, *sem_wd_2, *sem_wd_3;          // Semaphores for WD
sem_t *sem_logs_1, *sem_logs_2, *sem_logs_3;    // Semaphores for logger

// Sockets
int obstacles_sockfd;
int targets_sockfd;
int sockfd;
int newsockfd, portno, clilen, pid;

char logfile[80]; // path to logfile
char msg[1024];


int main(int argc, char *argv[]) 
{   
    // Read the file descriptors from the arguments
    get_args(argc, argv);

    // Read the config.txt file
    char program_type[MSG_LEN];
    char socket_data[MSG_LEN];
    read_args_from_file("./src/config.txt", program_type, socket_data);
    char host_name[MSG_LEN];
    int port_number;
    sprintf(msg, "Program type: %s", program_type);
    log_msg(logfile, SERVER, msg);

    parse_host_port(socket_data, host_name, &port_number);
    sprintf(msg, "Host name: %s\n", host_name);
    log_msg(logfile, SERVER, msg);

    sprintf(msg, "Port number: %d\n", port_number);
    log_msg(logfile, SERVER, msg);

    // Signals
    struct sigaction sa;
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO;
    sigaction (SIGINT, &sa, NULL);    
    sigaction (SIGUSR1, &sa, NULL);

    //////////////////////////////////////////////////////
    /* SHARED MEMORY and SEMAPHORES INITIALIZATION */
    /////////////////////////////////////////////////////

    /* Semaphores are unlocked by WD in order to get pids */

    // WATCHDOG
    ptr_wd = create_shm(SHM_WD);
    sem_wd_1 = sem_open(SEM_WD_1, O_CREAT, S_IRUSR | S_IWUSR, 0);
    if (sem_wd_1 == SEM_FAILED) {log_err(logfile, SERVER, "sem_wd_1 failed"); exit(1);}

    sem_wd_2 = sem_open(SEM_WD_2, O_CREAT, S_IRUSR | S_IWUSR, 0);
    if (sem_wd_2 == SEM_FAILED) {log_err(logfile, SERVER, "sem_wd_2 failed"); exit(1);}
    
    sem_wd_3 = sem_open(SEM_WD_3, O_CREAT, S_IRUSR | S_IWUSR, 0);
    if (sem_wd_3 == SEM_FAILED) {log_err(logfile, SERVER, "sem_wd_3 failed"); exit(1);}

    // LOGGER
    ptr_logs = create_shm(SHM_LOGS);
    sem_logs_1 = sem_open(SEM_LOGS_1, O_CREAT, S_IRUSR | S_IWUSR, 0); 
    if (sem_logs_1 == SEM_FAILED) {log_err(logfile, SERVER, "sem_logs_1 failed"); exit(1);}

    sem_logs_2 = sem_open(SEM_LOGS_2, O_CREAT, S_IRUSR | S_IWUSR, 0); 
    if (sem_logs_2 == SEM_FAILED) {log_err(logfile, SERVER, "sem_logs_2 failed"); exit(1);}

    sem_logs_3 = sem_open(SEM_LOGS_3, O_CREAT, S_IRUSR | S_IWUSR, 0); 
    if (sem_logs_3 == SEM_FAILED) {log_err(logfile, SERVER, "sem_logs_3 failed"); exit(1);}

    publish_pid_to_wd(SERVER_SYM, getpid());

    //////////////////////////////////////////////////////
    /* SOCKET CREATION */
    /////////////////////////////////////////////////////

    int obstacles_sockfd = 0;
    int targets_sockfd = 0;
    int sockfd = 0, newsockfd = 0, portno, clilen, pid;
    struct sockaddr_in serv_addr, cli_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {log_err(logfile, SERVER, "ERROR opening socket");}

    portno = port_number;  // Obtained from config.txt

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
        log_err(logfile, SERVER, "ERROR on binding");
    }

    listen(sockfd,5);
    clilen = sizeof(cli_addr);

    //////////////////////////////////////////////////////
    /* HANDLE CLIENT CONNECTIONS */
    /////////////////////////////////////////////////////
  
    while(targets_sockfd == 0 || obstacles_sockfd == 0){
        block_signal(SIGUSR1);
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        unblock_signal(SIGUSR1);
        if (newsockfd < 0) {log_err(logfile, SERVER, "ERROR on accept");}

        char socket_msg[MSG_LEN];
        read_then_echo(newsockfd, socket_msg);
        log_msg(logfile, "SOCKET", socket_msg);

        if (socket_msg[0] == 'O'){obstacles_sockfd = newsockfd;}
        else if(socket_msg[0] == 'T'){targets_sockfd = newsockfd;}
        else {close(newsockfd);}
    }

    // Variable: To compare current and previous data
    char prev_drone_msg[MSG_LEN] = "";


    while (1)
    {
        //////////////////////////////////////////////////////
        /* Handle pipe from key_manager.c */
        /////////////////////////////////////////////////////
        
        char km_msg[MSG_LEN];
        if (read_pipe_unblocked(km_server[0], km_msg) == 1){
            // Read acknowledgement
            sprintf(msg, "[PIPE] Received from key_manager.c: %s\n", km_msg);
            log_msg(logfile, SERVER, msg);

            fflush(stdout);
            // STOP key pressed (P)
            if (km_msg[0] == 'S'){
                write_then_wait_echo(targets_sockfd, km_msg, sizeof(km_msg),logfile, SERVER);
                write_then_wait_echo(obstacles_sockfd, km_msg, sizeof(km_msg), logfile, SERVER);
                exit(0);
            }
            // Response
            char response_km_msg[MSG_LEN*2];
            sprintf(response_km_msg, "K:%s", km_msg);
            write_to_pipe(server_drone[1], response_km_msg);
            sprintf(msg, "[PIPE] Sent to drone.c: %s\n", response_km_msg);
            log_msg(logfile, SERVER, msg);
            fflush(stdout);
        }
        
        //////////////////////////////////////////////////////
        /* Handle pipe from interface.c */
        /////////////////////////////////////////////////////
        
        char interface_msg[MSG_LEN];
        memset(interface_msg, 0 , MSG_LEN);

        if (read_pipe_unblocked(interface_server[0], interface_msg) == 1){
            // Read acknowledgement
            sprintf(msg, "[PIPE] Received from interface.c: %s\n", interface_msg);
            log_msg(logfile, SERVER, msg);
            fflush(stdout);
            // Response for drone.c
            write_to_pipe(server_drone[1], interface_msg);
            sprintf(msg, "[PIPE] Sent to drone.c: %s\n", interface_msg);
            log_msg(logfile, SERVER, msg);
            fflush(stdout);

            if(interface_msg[0] == 'I' && interface_msg[1] == '2'){
                // Send to socket: Client Targets
                char substring1[MSG_LEN];
                strcpy(substring1, interface_msg +3);
                write_then_wait_echo(targets_sockfd, substring1, sizeof(substring1), logfile, SERVER);
                // Send to socket: Client Obstacles
                char substring2[MSG_LEN];
                strcpy(substring2, interface_msg +3);
                write_then_wait_echo(obstacles_sockfd, substring2, sizeof(substring2), logfile, SERVER);
                sprintf(msg, "SENT %s to obstacles.c and targets.c\n", substring2);
                log_msg(logfile, SERVER, msg);

            }
            if (interface_msg[0] == 'G'){
                write_then_wait_echo(targets_sockfd, interface_msg, sizeof(interface_msg), logfile, SERVER);
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
                sprintf(msg, "[PIPE] Received from drone.c: %s\n", drone_msg);
                log_msg(logfile, SERVER, msg);

                fflush(stdout);
                // Update previous values
                strcpy(prev_drone_msg, drone_msg); 
                sprintf(msg,"[PIPE] Sent to interface.c: %s\n", drone_msg);
                log_msg(logfile, SERVER, msg);
            }
        }

        //////////////////////////////////////////////////////
        /* Handle socket from obstacles.c */
        /////////////////////////////////////////////////////

        char obstacles_msg[MSG_LEN];
        if (read_then_echo_unblocked(obstacles_sockfd, obstacles_msg, logfile, SERVER) == 1){
            // Send to interface.c
            write_to_pipe(server_interface[1],obstacles_msg);
            sprintf(msg,"[PIPE] Send to interface.c: %s\n", obstacles_msg);
            log_msg(logfile, SERVER, msg);

            fflush(stdout);
            // Send to drone.c
            write_to_pipe(server_drone[1], obstacles_msg);
            sprintf(msg, "[PIPE] Sent to drone.c: %s\n", obstacles_msg);
            log_msg(logfile, SERVER, msg);

            fflush(stdout);
        }

        //////////////////////////////////////////////////////
        /* Handle socket from targets.c */
        /////////////////////////////////////////////////////

        char targets_msg[MSG_LEN];
        if (read_then_echo_unblocked(targets_sockfd, targets_msg, logfile, SERVER) == 1)
        {
            // Send to interface.c
            write_to_pipe(server_interface[1],targets_msg);
            sprintf(msg, "[PIPE] Sent to interface.c: %s\n", targets_msg);
            log_msg(logfile, SERVER, msg);
        }
    }

    // Close and unlink
    clean_up();
    return 0;
}


void signal_handler(int signo, siginfo_t *siginfo, void *context) 
{
    // printf("Received signal number: %d \n", signo);
    if (signo == SIGINT)
    {
        sprintf(msg, "Caught SIGINT");
        log_msg(logfile, SERVER, msg);

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
        log_err(logfile, SERVER, "shm_open of shm_key");
        exit(1);
    }
    /* configure the size of the shared memory object */
    ftruncate(shm_fd, SIZE_SHM);

    /* memory map the shared memory segment */
    void *shm_ptr = mmap(0, SIZE_SHM, PROT_WRITE | PROT_READ, MAP_SHARED, shm_fd, 0); 
    if (shm_ptr == MAP_FAILED)
    {
        log_err(logfile, SERVER, "Map Failed");
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

    // Close all sockets
    close(sockfd);
    close(obstacles_sockfd);
    close(targets_sockfd);
    close(newsockfd);
    sprintf(msg, "Clean up has been performed succesfully");
    log_msg(logfile, SERVER, msg);

}


void get_args(int argc, char *argv[])
{
    sscanf(argv[1], "%d %d %d %d %d %d %d %d %d %s", &km_server[0], &server_drone[1], 
    &interface_server[0], &drone_server[0], &server_interface[1],
    &server_obstacles[1], &obstacles_server[0], &server_targets[1],
    &targets_server[0], logfile);
}
