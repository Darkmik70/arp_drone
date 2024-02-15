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


    while (1)
    {
        //////////////////////////////////////////////////////
        /* Handle pipe from key_manager.c */
        /////////////////////////////////////////////////////
        
        fd_set read_km;
        FD_ZERO(&read_km);
        FD_SET(km_server[0], &read_km);

        char km_msg[MSG_LEN];

        int ready_km = select(km_server[0] + 1, &read_km, NULL, NULL, &timeout);
        if (ready_km == -1) {perror("Error in select");}

        if (ready_km > 0 && FD_ISSET(km_server[0], &read_km)) {
            ssize_t bytes_read_km = read(km_server[0], km_msg, MSG_LEN);
            if (bytes_read_km > 0) {
                // Read acknowledgement
                printf("RECEIVED %s from key_manager.c\n", km_msg);
                fflush(stdout);
                // Response
                char response_km_msg[MSG_LEN*2];
                sprintf(response_km_msg, "K:%s", km_msg);
                write_to_pipe(server_drone[1], response_km_msg);
                printf("SENT %s to drone.c\n", response_km_msg);
                fflush(stdout);
            }
        }

        //////////////////////////////////////////////////////
        /* Handle pipe from interface.c */
        /////////////////////////////////////////////////////
        
        fd_set read_interface;
        FD_ZERO(&read_interface);
        FD_SET(interface_server[0], &read_interface);
        
        char interface_msg[MSG_LEN];

        int ready_interface = select(interface_server[0] + 1, &read_interface, NULL, NULL, &timeout);
        if (ready_interface == -1) {perror("Error in select");}

        if (ready_interface > 0 && FD_ISSET(interface_server[0], &read_interface)) {
            ssize_t bytes_read_interface = read(interface_server[0], interface_msg, MSG_LEN);
            if (bytes_read_interface > 0) {
                // Read acknowledgement
                printf("RECEIVED %s from interface.c\n", interface_msg);
                fflush(stdout);
                // Response for drone.c
                write_to_pipe(server_drone[1], interface_msg);
                printf("SENT %s to drone.c\n", interface_msg);
                fflush(stdout);
                // Response for obstacles.c and targets.c
                if(interface_msg[0] == 'I' && interface_msg[1] == '2'){
                    write_to_pipe(server_obstacles[1],interface_msg);
                    write_to_pipe(server_targets[1],interface_msg);
                    printf("SENT %s to obstacles.c and targets.c\n", interface_msg);
                }
            }
        }

        //////////////////////////////////////////////////////
        /* Handle pipe from drone.c */
        /////////////////////////////////////////////////////
        fd_set read_drone;
        FD_ZERO(&read_interface);
        FD_SET(drone_server[0], &read_drone);
        
        char drone_msg[MSG_LEN];

        int ready_drone = select(drone_server[0] + 1, &read_drone, NULL, NULL, &timeout);
        if (ready_drone == -1) {perror("Error in select");}

        if (ready_drone > 0 && FD_ISSET(drone_server[0], &read_drone)) {
            ssize_t bytes_read_drone = read(drone_server[0], drone_msg, MSG_LEN);
            if (bytes_read_drone > 0) {
                write_to_pipe(server_interface[1], drone_msg);
                if (strcmp(drone_msg, prev_drone_msg) != 0) {
                    printf("RECEIVED %s from drone.c\n", drone_msg);
                    fflush(stdout);
                    strcpy(prev_drone_msg, drone_msg); // Update previous values
                    printf("SENT %s to interface.c\n", drone_msg);
                    fflush(stdout);
                }
            }
            else if (bytes_read_drone == -1) {perror("Read pipe drone_server");}
        }

        //////////////////////////////////////////////////////
        /* Handle pipe from obstacles.c */
        /////////////////////////////////////////////////////

        fd_set read_obstacles;
        FD_ZERO(&read_obstacles);
        FD_SET(obstacles_server[0], &read_obstacles);
        
        char obstacles_msg[MSG_LEN];

        int ready_obstacles = select(obstacles_server[0] + 1, &read_obstacles, NULL, NULL, &timeout);
        if (ready_obstacles == -1) {perror("Error in select");}

        if (ready_obstacles > 0 && FD_ISSET(obstacles_server[0], &read_obstacles)) {
            ssize_t bytes_read_obstacles = read(obstacles_server[0], obstacles_msg, MSG_LEN);
            if (bytes_read_obstacles > 0) {
                // Read acknowledgement
                printf("RECEIVED %s from obstacles.c\n", obstacles_msg);
                fflush(stdout);
                // Send to interface.c
                write_to_pipe(server_interface[1],obstacles_msg);
                printf("SENT %s to interface.c\n", obstacles_msg);
                fflush(stdout);
                // Send to drone.c
                write_to_pipe(server_drone[1], obstacles_msg);
                printf("SENT %s to drone.c\n", obstacles_msg);
                fflush(stdout);
            }
            else if (bytes_read_obstacles == -1) {perror("Read pipe obstacles_server");}
        }

        //////////////////////////////////////////////////////
        /* Handle pipe from targets.c */
        /////////////////////////////////////////////////////

        fd_set read_targets;
        FD_ZERO(&read_targets);
        FD_SET(targets_server[0], &read_targets);
        
        char targets_msg[MSG_LEN];

        int ready_targets = select(targets_server[0] + 1, &read_targets, NULL, NULL, &timeout);
        if (ready_targets == -1) {perror("Error in select");}

        if (ready_targets > 0 && FD_ISSET(targets_server[0], &read_targets)) {
            ssize_t bytes_read_targets = read(targets_server[0], targets_msg, MSG_LEN);
            if (bytes_read_targets > 0) {
                // Read acknowledgement
                printf("RECEIVED %s from targets.c\n", targets_msg);
                fflush(stdout);
                // Send to interface.c
                write_to_pipe(server_interface[1],targets_msg);
                printf("SENT %s to interface.c\n", targets_msg);
                fflush(stdout);
            }
            else if (bytes_read_targets == -1) {perror("Read pipe targets_server");}
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
    // close all connections
    sem_close(sem_wd_1);
    sem_close(sem_wd_2);
    sem_close(sem_wd_3);

    // unlink semaphores
    sem_unlink(SEM_WD_1);
    sem_unlink(SEM_WD_2);
    sem_unlink(SEM_WD_3);

    // unmap shared memory
    munmap(ptr_wd, SIZE_SHM);

    // unlink shared memories
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