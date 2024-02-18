#ifndef CONSTANTS_H
#define CONSTANTS_H

// PORT NUMBER FOR SOCKETS
#define PORT_NUMBER 3000

// General message length
#define MSG_LEN 1024

// Specific message length for logfiles
#define LOG_LEN 256

// Shared mem for Watchdog's pid
#define SHM_WD "/wd_pid_1"
#define SEM_WD_1 "/sem_wd_1"
#define SEM_WD_2 "/sem_wd_2"
#define SEM_WD_3 "/sem_wd_3"
// Shared memory key for LOGS
#define SHM_LOGS "/shared_logs"
#define SEM_LOGS_1 "/my_semaphore_logs_1"
#define SEM_LOGS_2 "/my_semaphore_logs_2"
#define SEM_LOGS_3 "/my_semaphore_logs_3"

// SYMBOLS FOR WATCHDOG and LOGGER
#define SERVER_SYM 1
#define WINDOW_SYM 2
#define KM_SYM 3
#define DRONE_SYM 4
#define LOGGER_SYM 5
#define WD_SYM 6

// SYMBOLS FOR LOG TYPES
#define INFO 1
#define WARN 2
#define ERROR 3

// General size of Shared memory (in bytes)
#define SIZE_SHM 4096 

// Struct to save obstacle's data
typedef struct {
    int total;
    int x;
    int y;
} Obstacles;

// Struct to save target's data
typedef struct {
    int id;
    int x;
    int y;
} Targets;


#endif // CONSTANTS_H