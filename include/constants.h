#ifndef CONSTANTS_H
#define CONSTANTS_H

// Shared mem for Watchdog's pid
#define SHM_WD "/wd_pid_1"
#define SEM_WD_1 "/sem_wd_1"
#define SEM_WD_2 "/sem_wd_2"
#define SEM_WD_3 "/sem_wd_3"


// Shared memory key for key presses
#define SHM_KEY "/shared_keyboard"
#define SEM_KEY "/my_semaphore_1"

// Shared memory key for drone positions
#define SHM_POS "/shared_drone_pos"
#define SEM_POS "/my_semaphore_2"

// Shared memory key for drone control (actions).
#define SHM_ACTION "/shared_control"
#define SEM_ACTION "/my_semaphore_3"

// Shared memory key for LOGS
#define SHM_LOGS "/shared_logs"
#define SEM_LOGS "/my_semaphore_4"

// Pipes
#define MSG_LEN  80

// Length for logfiles
#define LOG_LEN 256


// Special value to indicate no key pressed
#define NO_KEY_PRESSED 0

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


#define SIZE_SHM 4096 // General size of Shared memory (in bytes)
#define FLOAT_TOLERANCE 0.0044 //TODO: ADD WHAT IT MEANS

#endif // CONSTANTS_H