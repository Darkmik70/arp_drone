#include "watchdog.h"
#include "common.h"

/* Global variables */
pid_t server_pid;
pid_t interface_pid;
pid_t km_pid;
pid_t drone_pid;
pid_t obstacles_pid;
pid_t targets_pid;

pid_t wd_pid;
pid_t logger_pid;

/* Counters for components*/
int cnt_server;
int cnt_window;
int cnt_km;
int cnt_drone;
int cnt_obstacles;
int cnt_targets;

int cnt_logger;


void signal_handler(int signo, siginfo_t *siginfo, void *context)
{
    printf("Received signal number: %d\n from ", signo);
    if (signo == SIGINT) 
        send_sigint_to_all(); 
    if (signo == SIGUSR2)
    {
        // CHECK THE PID OF SENDER AND SET COUNTERS TO ZERO
        if (siginfo->si_pid == server_pid)
        {
            printf("Server has sent SIGUSR2 \n\n");
            cnt_server = 0;
        }
        if (siginfo->si_pid == interface_pid)
        {
            printf("Interface has sent SIGUSR2 \n\n");
            cnt_window = 0;
        }
        if (siginfo->si_pid == km_pid)
        {
            printf("Key Manager has sent SIGUSR2 \n\n");
            cnt_km = 0;
        }
        if (siginfo->si_pid == drone_pid)
        {
            printf("Drone has sent SIGUSR2 \n\n");
            cnt_drone = 0;
        }
        if (siginfo->si_pid == logger_pid)
        {
            printf("Logger has sent SIGUSR2 \n\n");
            cnt_logger = 0;
        }
        if (siginfo->si_pid == obstacles_pid)
        {
            printf("Obstacles has sent SIGUSR2 \n\n");
            cnt_obstacles = 0;
        }
        if (siginfo->si_pid == targets_pid)
        {
            printf("Targets has sent SIGUSR2 \n\n");
            cnt_obstacles = 0;
        }
    }
 }


int main(int argc, char* argv[])
{
    /* Sigaction */
    struct sigaction sa;
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);    
    sigaction(SIGUSR2, &sa, NULL);    

    /* Get PIDs of processes*/
    server_pid = 0;
    interface_pid = 0;
    km_pid = 0;
    drone_pid = 0;
    logger_pid = 0;
    obstacles_pid = 0;
    targets_pid = 0;
    wd_pid = getpid();

    // Get pid of the processes
    get_pids(&server_pid, &interface_pid, &km_pid, &drone_pid, &obstacles_pid, &targets_pid);
    printf(" PID of WD: %d\n", wd_pid);

    /*TODO: remove hardcoded values in WATCHDOG */
    cnt_server = 0;
    cnt_window = 0;
    cnt_km = 0;
    cnt_drone = 0;
    cnt_logger = 0;
    cnt_obstacles = 0;
    cnt_targets = 0;

    // while(1)
    // {
    //     // FIXME
    //     // increment counter
    //     cnt_server++;
    //     cnt_window++;
    //     cnt_km++;
    //     cnt_drone++;
    //     cnt_obstacles++;
    //     cnt_targets++;
    //     /* cnt_logger++; */

    //     /* Monitor health of all of the processes */
    //     kill(server_pid, SIGUSR1);
    //     usleep(500);
    //     kill(interface_pid, SIGUSR1);
    //     usleep(500);
    //     kill(km_pid, SIGUSR1);
    //     usleep(500);
    //     kill(drone_pid, SIGUSR1);
    //     usleep(500);
    //     kill(targets_pid, SIGUSR1);
    //     usleep(500);
    //     kill(obstacles_pid, SIGUSR1);
    //     usleep(500);

    //     /* kill(logger_pid, SIGUSR1); */



    //     // If any of the processess does not respond in given timeframe, close them all
    //     if (cnt_server > THRESHOLD || cnt_window > THRESHOLD || cnt_km > THRESHOLD || cnt_drone > THRESHOLD ||
    //         cnt_targets > THRESHOLD || cnt_obstacles > THRESHOLD /*|| cnt_logger > THRESHOLD */)
    //     {
    //         send_sigint_to_all();
    //     }
    //     usleep(1000000);
    // }

    return 0;
}


int get_pids(pid_t *server_pid, pid_t *interface_pid, pid_t *km_pid, 
                pid_t *drone_pid, pid_t *obstacles_pid, pid_t *targets_pid)
{
    sem_t *sem_wd_1 = sem_open(SEM_WD_1, 0);
    sem_t *sem_wd_2 = sem_open(SEM_WD_2, 0);
    sem_t *sem_wd_3 = sem_open(SEM_WD_3, 0);

    int shm_wd_fd = shm_open(SHM_WD, O_RDWR, 0666);
    char *ptr_wd = mmap(0, SIZE_SHM, PROT_READ | PROT_WRITE, MAP_SHARED, shm_wd_fd, 0);


    // give initial 'signal' to processes that WD is ready to get the pids
    sem_post(sem_wd_1);
    for (int i = 0; i < 6; i++)
    {   
        // wait for a signal from processes that you can acccess pids
        sem_wait(sem_wd_2);

        // Set the pids depending on the symbol
        int symbol = 0;
        pid_t pid_temp;
        sscanf(ptr_wd, "%i %i", &symbol, &pid_temp);
        // printf("current pid %d  symbol %i \n", pid_temp, symbol);
        switch (symbol)
        {
        case SERVER_SYM:
            *server_pid = pid_temp;
            printf("Server PID SET\n");
            break;
        case WINDOW_SYM:
            *interface_pid = pid_temp;
            printf("Interface PID SET\n");
            break;
        case KM_SYM:
            *km_pid = pid_temp;
            printf("Key Manager PID SET\n");
            break;
        case DRONE_SYM:
            *drone_pid = pid_temp;
            printf("Drone PID SET\n");
            break;
        case OBSTACLES_SYM:
            *obstacles_pid = pid_temp;
            printf("Obstacles PID SET\n");
            break;
        case TARGETS_SYM:
            *targets_pid = pid_temp;
            printf("Targets PID SET\n");
            break;
        // case LOGGER_SYM:
        //     *logger_pid = pid_temp;
        //     printf("LOGGER PID SET\n");
        //     break;
        default:
            perror("Wrong process symbol!");
            exit(1);
        }

        // clear memory to make sure we got everything we need
        memset(ptr_wd, 0, SIZE_SHM);
        
        // signal process to unlock the first semaphore
        sem_post(sem_wd_3);
    }
    
    if (*server_pid != 0 && *interface_pid != 0 && *km_pid != 0 && *drone_pid != 0 
        && *obstacles_pid != 0 && *targets_pid != 0) 
    {
        // Got all pids
        printf("Those are obtained pids: \n");
        printf("SERVER : %i \n WINDOW : %i \n KM : %i \n DRONE : %i \n" 
                "OBSTACLES : %i \n TARGETS : %i \n", 
                *server_pid, *interface_pid, *km_pid, *drone_pid, 
                *obstacles_pid, *targets_pid);
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

void send_sigint_to_all()
{
    kill(interface_pid, SIGINT);
    kill(km_pid, SIGINT);
    kill(drone_pid, SIGINT);
    kill(server_pid, SIGINT);
    kill(obstacles_pid, SIGINT);
    kill(targets_pid, SIGINT);
    // kill(logger_pid, SIGINT);
    printf("WD sent SIGINT to all processes, exiting...\n");
    exit(1);
}

