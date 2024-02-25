#include "common.h"
#include <signal.h>

// block the sspecified signal signal
void block_signal(int signal)
{
    // Create a set of signals to block
    sigset_t sigset;
    // Initialize the set to 0
    sigemptyset(&sigset);
    // Add the signal to the set
    sigaddset(&sigset, signal);
    // Add the signals in the set to the process' blocked signals
    sigprocmask(SIG_BLOCK, &sigset, NULL);
}

void unblock_signal(int signal)
{   
    // Create a set of signals to unblock
    sigset_t sigset;
    // Initialize the set to 0
    sigemptyset(&sigset);
    // Add the signal to the set
    sigaddset(&sigset, signal);
    // Remove the signals in the set from the process' blocked signals
    sigprocmask(SIG_UNBLOCK, &sigset, NULL);
}

// Publish the PID integer value to the watchdog.
void publish_pid_to_wd(int process_symbol, pid_t pid)
{   
    int shm_wd_fd = shm_open(SHM_WD, O_RDWR, 0666);
    char *ptr_wd = mmap(0, SIZE_SHM, PROT_READ | PROT_WRITE, MAP_SHARED, shm_wd_fd, 0);
    sem_t *sem_wd_1 = sem_open(SEM_WD_1, 0);    // Protects shm_wd
    sem_t *sem_wd_2 = sem_open(SEM_WD_2, 0);    // Allows WD to use SHM_WD
    sem_t *sem_wd_3 = sem_open(SEM_WD_3, 0);    // WD allows processes to unlock sem_wd_1

    // Wait for signal from WD / other process to give your pid to WD
    sem_wait(sem_wd_1);
    snprintf(ptr_wd, sizeof(ptr_wd), "%i %i", process_symbol, pid);   
    // shm is set, signal WD so it can get the PID
    sem_post(sem_wd_2);
    // Wait for response from WD 
    sem_wait(sem_wd_3);
    // Allow other processes to do that. 
    sem_post(sem_wd_1);
    // PID's published, close your connections
    sem_close(sem_wd_1);
    sem_close(sem_wd_2);
    sem_close(sem_wd_3);
    // Detach from shared memorry
    munmap(ptr_wd,SIZE_SHM);
}


void log_msg(char *filepath, char* who, char *message)
{
    // Open the file
    FILE *file_fd = fopen(filepath, "a");
    if (file_fd == NULL)
    {
        perror("Logfile open failed!");
        exit(1);
    }
    
    // Get the current time
    time_t currentTime;
    time(&currentTime);
    struct tm *localTime = localtime(&currentTime);
    int hours = localTime->tm_hour;
    int minutes = localTime->tm_min;
    int seconds = localTime->tm_sec;
    
    char time[80];
    char *eol = "\n";
    sprintf(time, "%02d:%02d:%02d", hours, minutes, seconds);
    
    fprintf(file_fd, "[INFO][%s] at [%s:] %s", who, time, message);
    fclose(file_fd); //close file at the end
    message = ""; // clear the buffer
}

void log_err(char *filepath, char* who, char *message)
{
    perror(message);
    // Open the file
    FILE *file_fd = fopen(filepath, "a");
    if (file_fd == NULL)
    {
        perror("Logfile open failed!");
        exit(1);
    }
    
    // Get the current time
    time_t currentTime;
    time(&currentTime);
    struct tm *localTime = localtime(&currentTime);
    int hours = localTime->tm_hour;
    int minutes = localTime->tm_min;
    int seconds = localTime->tm_sec;
    
    char time[80];
    char *eol = "\n";
    sprintf(time, "%02d:%02d:%02d", hours, minutes, seconds);
    
    fprintf(file_fd, "[ERROR][%s] at [%s:] %s", who, time, message);
    fclose(file_fd); //close file at the end
    message = ""; // clear the buffer
}

    // Writes message using the file descriptor provided.
void write_to_pipe(int pipe_fd, char message[])
{
    ssize_t bytes_written = write(pipe_fd, message, MSG_LEN);
    if (bytes_written == -1)
    {
        perror("Write went wrong");
        exit(1);
    }
}


// Reads a message from the socket, then does an echo.
void read_then_echo(int sockfd, char socket_msg[]){
    int bytes_read, bytes_written;
    bzero(socket_msg, MSG_LEN);

    // READ from the socket
    block_signal(SIGUSR1);
    bytes_read = read(sockfd, socket_msg, MSG_LEN - 1);
    unblock_signal(SIGUSR1);
    if (bytes_read < 0) perror("ERROR reading from socket");
    else if (bytes_read == 0) {return;}  // Connection closed
    else if (socket_msg[0] == '\0') {return;} // Empty string
    // printf("[SOCKET] Received: %s\n", socket_msg);
    
    // ECHO data into socket
    block_signal(SIGUSR1);
    bytes_written = write(sockfd, socket_msg, bytes_read);
    unblock_signal(SIGUSR1);
    if (bytes_written < 0) {perror("ERROR writing to socket");}
    // printf("[SOCKET] Echo sent: %s\n", socket_msg);
}

// Reads a message from the socket, with select() system call, then does an echo.
int read_then_echo_unblocked(int sockfd, char socket_msg[], char* logfile,char *log_who)
{
    char msg_logs[1024];
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
    block_signal(SIGUSR1);
    ready = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);
    unblock_signal(SIGUSR1);
    if (ready < 0) {perror("ERROR in select");} 
    else if (ready == 0) {return 0;}  // No data available
    else
    {
        // Data is available for reading, so read from the socket
        bytes_read = read(sockfd, socket_msg, MSG_LEN - 1);
        if (bytes_read < 0) {perror("ERROR reading from socket");} 
        else if (bytes_read == 0) {return 0;}  // Connection closed
        else if (socket_msg[0] == '\0') {return 0;} // Empty string

        sprintf(msg_logs, "[SOCKET] Received: %s\n", socket_msg);
        log_msg(logfile, log_who, msg_logs);
    }


    // Echo the message back to the client
    block_signal(SIGUSR1);
    bytes_written = write(sockfd, socket_msg, bytes_read);
    unblock_signal(SIGUSR1);
    if (bytes_written < 0) {perror("ERROR writing to socket");}
    else
    { 
    sprintf(msg_logs,"[SOCKET] Echo sent: %s\n", socket_msg);
    log_msg(logfile, log_who, msg_logs);
    return 1;
    }
}

// Writes a message into the socket, then loops/waits until a valid echo is read.
void write_then_wait_echo(int sockfd, char socket_msg[], size_t msg_size, char *logfile, char* log_who){
    int correct_echo = 0;
    int bytes_read, bytes_written;
    char response_msg[MSG_LEN];
    char msg[1024];

    while(correct_echo == 0){
        block_signal(SIGUSR1);
        bytes_written = write(sockfd, socket_msg, msg_size);
        unblock_signal(SIGUSR1);
        if (bytes_written < 0) {perror("ERROR writing to socket");}
        sprintf(msg, "[SOCKET] Sent: %s\n", socket_msg);
        log_msg(logfile, log_who, msg);
        // Clear the buffer
        bzero(response_msg, MSG_LEN);

        while (response_msg[0] == '\0'){
            // Data is available for reading, so read from the socket
            block_signal(SIGUSR1);
            bytes_read = read(sockfd, response_msg, bytes_written);
            unblock_signal(SIGUSR1);
            if (bytes_read < 0) {perror("ERROR reading from socket");} 
            else if (bytes_read == 0) {printf("Connection closed!\n"); return;}
        }

        if (strcmp(socket_msg, response_msg) == 0){
            // Print the received message
            sprintf(msg, "[SOCKET] Echo received: %s\n", response_msg);
            log_msg(logfile, log_who, msg);
            correct_echo = 1;
        }
    }
}

// Reads a message from the pipe with select() system call.
int read_pipe_unblocked(int pipefd, char msg[]){
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    fd_set read_pipe;
    FD_ZERO(&read_pipe);
    FD_SET(pipefd, &read_pipe);

    char buffer[MSG_LEN];

    block_signal(SIGUSR1);
    int ready_km = select(pipefd + 1, &read_pipe, NULL, NULL, &timeout);
    unblock_signal(SIGUSR1);
    if (ready_km == -1)
    {
        perror("Error in select");
    }

    if (ready_km > 0 && FD_ISSET(pipefd, &read_pipe)) {
        ssize_t bytes_read_pipe = read(pipefd, buffer, MSG_LEN);
        if (bytes_read_pipe > 0) {
            strcpy(msg, buffer);
            return 1;
        }
        else{return 0;}
    }
}

// Reads the first line of uncommented text from a file
void read_args_from_file(const char *filename, char *type, char *data)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file %s\n", filename);
        // exit(EXIT_FAILURE);
    }

    // Read lines until finding the first non-comment line
    char line[MSG_LEN];
    while (fgets(line, sizeof(line), file) != NULL) {
        // Skip lines starting with '#' (comments)
        if (line[0] != '#') {
            // Tokenize the line to extract type and data
            char *token = strtok(line, " \t\n");
            if (token != NULL) {
                strncpy(type, token, MSG_LEN);
                token = strtok(NULL, " \t\n");
                if (token != NULL) {
                    strncpy(data, token, MSG_LEN);
                    fclose(file);
                    return;
                }
            }
        }
    }
}

// Obtains both the host name and the port number from a formatted string
void parse_host_port(const char *str, char *host, int *port) {
    char *colon = strchr(str, ':');
    if (colon == NULL) {
        fprintf(stderr, "Invalid input: No colon found\n");
        exit(EXIT_FAILURE);
    }

    int len = colon - str;
    strncpy(host, str, len);
    host[len] = '\0';

    *port = atoi(colon + 1);
}




