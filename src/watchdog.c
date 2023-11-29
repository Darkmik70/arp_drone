#include "watchdog.h"
#include "constants.h"

#include <stdio.h>  
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>

// GLOBAL VARIABLES
pid_t server_pid;
pid_t window_pid;
pid_t km_pid;
pid_t drone_pid;
pid_t wd_pid;



void signal_handler(int signo) {
    // Handle the signal, e.g., print a message
    printf("Received signal number: %d\n", signo);

    if (signo == SIGINT)
    {
        // Send sigint to all of the processess;
        kill(window_pid, SIGINT);
        kill(km_pid, SIGINT);
        kill(drone_pid, SIGINT);
        kill(server_pid, SIGINT);

        printf("WD sent SIGINT to all processes, exiting...\n");
        exit(1);
    } 
 }


int main(int argc, char* argv[])
{
    /* Sigaction */
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigaction(SIGINT, &sa, NULL);    

    /* Get PIDs of processes*/
    server_pid = 0;
    window_pid = 0;
    km_pid = 0;
    drone_pid = 0;
    wd_pid = getpid();


    // Get pid of the processes
    get_pids(&server_pid, &window_pid, &km_pid, &drone_pid);
    printf("The PID of this process is: %d\n", wd_pid);

    printf("Those are obtained pids: \n SERVER : %i \n WINDOW : %i \n KM : %i \n DRONE : %i \n", server_pid, window_pid, km_pid, drone_pid);

    /*TODO user signal */

    while(1)
    {
        /* Monitor health of all of the processes*/
        // kill(server_pid, SIGUSR1);

        // kill(window_pid, SIGUSR1);
        // kill(km_pid, SIGUSR1);
        // kill(drone_pid, SIGUSR1);

        usleep(1000000);

    }

    // clean_up_before_exit();
    return 0;
}


int get_pids(pid_t *server_pid, pid_t *window_pid, pid_t *km_pid, pid_t *drone_pid)
{
    sem_t *sem_wd_1 = sem_open(SEM_WD_1, 0);
    sem_t *sem_wd_2 = sem_open(SEM_WD_2, 0);
    sem_t *sem_wd_3 = sem_open(SEM_WD_3, 0);

    int shm_wd_fd = shm_open(SHM_WD, O_RDWR, 0666);
    char *ptr_wd = mmap(0, SIZE_SHM, PROT_READ | PROT_WRITE, MAP_SHARED, shm_wd_fd, 0);


    // give initial 'signal' to processes that WD is ready to get the pids
    sem_post(sem_wd_1);
    for (int i = 0; i < 4; i++)
    {   
        // wait for a signal from processes that you can acccess pids
        sem_wait(sem_wd_2);

        // Set the pids depending on the symbol
        int symbol = 0;
        pid_t pid_temp;
        sscanf(ptr_wd, "%i %i", &symbol, &pid_temp);
        switch (symbol)
        {
        case SERVER_SYM:
            *server_pid = pid_temp;
            printf("Server PID SET\n");
            break;
        case WINDOW_SYM:
            *window_pid = pid_temp;
            printf("WINDOW PID SET\n");
            break;
        case KM_SYM:
            *km_pid = pid_temp;
            printf("KEYMANAGER PID SET\n");
            break;
        case DRONE_SYM:
            *drone_pid = pid_temp;
            printf("DRONE PID SET\n");
            break;
        default:
            perror("Wrong process symbol!");
            exit(1);
        }

        // clear memory to make sure we got everything we need
        memset(ptr_wd, 0, SIZE_SHM);
        
        // signal process to unlock the first semaphore
        sem_post(sem_wd_3);
    }
    
    if (*server_pid != 0 && *window_pid != 0 && *km_pid != 0 && *drone_pid != 0)
    {
        // Got all pids
        printf("Those are obtained pids: \n");
        printf(" SERVER : %i \n WINDOW : %i \n KM : %i \n DRONE : %i \n", 
            *server_pid, *window_pid, *km_pid, *drone_pid);
    }
    else
    {
        // At this point all the values should be set
        perror("Not all of the pids have been set");
        exit(1);
    }
        // close semaphores - no use for them anymore
        sem_close(sem_wd_1);
        sem_close(sem_wd_2);
        sem_close(sem_wd_3);

        // unmap the memory segment
        munmap(ptr_wd,SIZE_SHM); 
        //TODO send signal to server to unlink wd stuff
}


