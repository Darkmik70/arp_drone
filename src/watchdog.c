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

char logfile[80]; // path to logfile
char msg[1024];



void signal_handler(int signo, siginfo_t *siginfo, void *context)
{
    sprintf(msg, "Received signal number: %d\n from ", signo);
    log_msg(logfile, WD, msg);
    if (signo == SIGINT)
        send_sigint_to_all(); 
    if (signo == SIGUSR2)
    {
        // CHECK THE PID OF SENDER AND SET COUNTERS TO ZERO
        if (siginfo->si_pid == server_pid)
        {
            sprintf(msg, "Received SIGUSR2 from Server\n");
            log_msg(logfile, WD, msg);
            cnt_server = 0;
        }
        if (siginfo->si_pid == interface_pid)
        {
            sprintf(msg, "Received SIGUSR2 from Interface \n");
            log_msg(logfile, WD, msg);
            cnt_window = 0;
        }
        if (siginfo->si_pid == km_pid)
        {
            sprintf(msg, "Received SIGUSR2 from Key Manager \n");
            log_msg(logfile, WD, msg);
            cnt_km = 0;
        }
        if (siginfo->si_pid == drone_pid)
        {
            sprintf(msg, "Received SIGUSR2 from Drone\n");
            log_msg(logfile, WD, msg);
            cnt_drone = 0;
        }
        if (siginfo->si_pid == logger_pid)
        {
            sprintf(msg, "Received SIGUSR2 from Logger \n");
            log_msg(logfile, WD, msg);
            cnt_logger = 0;
        }
        if (siginfo->si_pid == obstacles_pid)
        {
            sprintf(msg, "Received SIGUSR2 from Obstacles \n");
            log_msg(logfile, WD, msg);
            cnt_obstacles = 0;
        }
        if (siginfo->si_pid == targets_pid)
        {
            sprintf(msg, "Received SIGUSR2 from Targets\n");
            log_msg(logfile, WD, msg);
            cnt_targets = 0;
        }
    }
 }


int main(int argc, char* argv[])
{
    get_args(argc, argv);

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
    sprintf(msg, "PID of WATCHDOG: %d\n", wd_pid);
    log_msg(logfile, WD, msg);

    // decide on mode
    char mode[80];
    read_args_from_file(CONFIG_PATH, mode, NULL);

    /*TODO: remove hardcoded values in WATCHDOG */
    cnt_server = 0;
    cnt_window = 0;
    cnt_km = 0;
    cnt_drone = 0;
    cnt_logger = 0;
    cnt_obstacles = 0;
    cnt_targets = 0;

    while (1)
    {
        // do diagnostics only for standalone
        if (strcmp(mode, "standalone") == 0)
        {
            // increment counter
            cnt_server++;
            cnt_window++;
            cnt_km++;
            cnt_drone++;
            // FIXME: Unknown problems with signal handling in obstacles and targets
            // cnt_obstacles++;
            // cnt_targets++;

            /* Monitor health of all of the processes */
            kill(server_pid, SIGUSR1);
            usleep(50);
            kill(interface_pid, SIGUSR1);
            usleep(50);
            kill(km_pid, SIGUSR1);
            usleep(50);
            kill(drone_pid, SIGUSR1);
            usleep(50);
            kill(targets_pid, SIGUSR1);
            usleep(50);
            kill(obstacles_pid, SIGUSR1);
            usleep(50);

            // If any of the processess does not respond in given timeframe, close them all
            if (cnt_server > THRESHOLD || cnt_window > THRESHOLD || cnt_km > THRESHOLD || cnt_drone > THRESHOLD ||
                cnt_targets > THRESHOLD || cnt_obstacles > THRESHOLD)
            {
                sprintf(msg, "One of the counters has went through threshold, Threshold value: %d, values - server: %d window : % d, key manager : % d, drone : % d, targets : % d, obstacles : % d",
                        THRESHOLD, cnt_server, cnt_window, cnt_km, cnt_drone, cnt_targets, cnt_obstacles);
                log_msg(logfile, WD, msg);
                send_sigint_to_all();
            }
        }
        usleep(1000000);
    }

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
        sprintf(msg,"current pid %d  symbol %i \n", pid_temp, symbol);
        log_msg(logfile, WD, msg);
        switch (symbol)
        {
        case SERVER_SYM:
            *server_pid = pid_temp;
            sprintf(msg, "Server PID SET: %d", pid_temp);
            log_msg(logfile, WD, msg);
            break;
        case WINDOW_SYM:
            *interface_pid = pid_temp;
            sprintf(msg,"Interface PID SET: %d", pid_temp);
            log_msg(logfile, WD, msg);
            break;
        case KM_SYM:
            *km_pid = pid_temp;
            sprintf(msg,"Key Manager PID SET: %d", pid_temp);
            log_msg(logfile, WD, msg);
            break;
        case DRONE_SYM:
            *drone_pid = pid_temp;
            sprintf(msg,"Drone PID SET: %d", pid_temp);
            log_msg(logfile, WD, msg);
            break;
        case OBSTACLES_SYM:
            *obstacles_pid = pid_temp;
            sprintf(msg,"Obstacles PID SET: %d", pid_temp);
            log_msg(logfile, WD, msg);
            break;
        case TARGETS_SYM:
            *targets_pid = pid_temp;
            sprintf(msg,"Targets PID SET: %d", pid_temp);
            log_msg(logfile, WD, msg);    
            break;
        default:
            sprintf(msg, "Wrong process symbol!: %d ", symbol);
            log_err(logfile, WD, msg);
            // exit(1);
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
        sprintf(msg, "Got all pids");
        log_msg(logfile, WD, msg);
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
    sprintf(msg, "WD sent SIGINT to all processes, exiting...");
    log_msg(logfile, WD, msg);
    exit(1);
}

void get_args(int argc, char *argv[])
{
    sscanf(argv[1], "%s", logfile);
}
