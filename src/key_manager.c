#include "key_manager.h"
#include "constants.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <semaphore.h>
#include <errno.h>

// Serverless pipes
int key_press_fd_read;
// Pipes working with the server
int km_server_write;

int main(int argc, char *argv[]) 
{
    get_args(argc, argv);

    struct sigaction sa;
    sa.sa_sigaction = signal_handler; 
    sa.sa_flags = SA_SIGINFO;
    sigaction (SIGINT, &sa, NULL);
    sigaction (SIGUSR1, &sa, NULL);   

    publish_pid_to_wd(KM_SYM, getpid());

    while (1)
    {
        /*THIS SECTION IS FOR OBTAINING THE KEY INPUT CHARACTER*/

        fd_set readset_km;
        // Initializes the file descriptor set readset by clearing all file descriptors from it.
        FD_ZERO(&readset_km);
        // Adds key_pressing to the file descriptor set readset.
        FD_SET(key_press_fd_read, &readset_km);
        
        int ready;
        // This waits until a key press is sent from (interface.c)
        do {
            ready = select(key_press_fd_read + 1, &readset_km, NULL, NULL, NULL);
            //if (ready == -1) {perror("Error in select");}
        } while (ready == -1 && errno == EINTR);
        
        // Read from the file descriptor
        int pressedKey = read_key_from_pipe(key_press_fd_read);
        printf("Pressed key: %c\n", (char)pressedKey);
        fflush(stdout);

        /*THIS SECTION IS FOR DRONE ACTION DECISION*/
        
        char *action = determineAction(pressedKey);
        fflush(stdout);

        // TEMPORAL/DELETE AFTER: TESTING DATA SENT TO PIPE ACTION
        char key = toupper(pressedKey);
        int x; int y;

        if ( action != "None")
        {
            write_to_pipe(km_server_write, action);
            printf("Wrote action message: %s into pipe\n", action);
        }
    }

    return 0;
}

int read_key_from_pipe(int pipe_fd)
{
    char msg[MSG_LEN];
    ssize_t bytes_read = read(pipe_fd, msg, sizeof(msg));
    int pressed_key = msg[0];
    return pressed_key;
}


// US Keyboard assumed
char* determineAction(int pressedKey)
{
    char key = toupper(pressedKey);
    int x; int y;

    // TODO: Every sprintf is a write into shared memory. Must be changed into the action pipe sent to (drone.c)

    // Disclaimer: Y axis is inverted on tested terminal.
    if ( key == 'W')
    {
        x = 0;    // Movement on the X axis.
        y = -1;    // Movement on the Y axis.
        return "0,-1";
    }
    if ( key == 'X')
    {
        x = 0;    // Movement on the X axis.
        y = 1;    // Movement on the Y axis.
        return "0,1";
    }
    if ( key == 'A')
    {
        x = -1;    // Movement on the X axis.
        y = 0;    // Movement on the Y axis.
        return "-1,0";
    }
    if ( key == 'D')
    {
        x = 1;    // Movement on the X axis.
        y = 0;    // Movement on the Y axis.
        return "1,0";
    }
    if ( key == 'Q')
    {
        x = -1;    // Movement on the X axis.
        y = -1;    // Movement on the Y axis.
        return "-1,-1";
    }
    if ( key == 'E')
    {
        x = 1;    // Movement on the X axis.
        y = -1;    // Movement on the Y axis.
        return "1,-1";
    }
    if ( key == 'Z')
    {
        x = -1;    // Movement on the X axis.
        y = 1;    // Movement on the Y axis.
        return "-1,1";
    }
    if ( key == 'C')
    {
        x = 1;    // Movement on the X axis.
        y = 1;    // Movement on the Y axis.
        return "1,1";
    }
    if ( key == 'S')
    {
        x = 900;    // Special value interpreted by drone.c process
        y = 0;
        return "900,0";
    }
    else
    {
        return "None";
    }

}

void get_args(int argc, char *argv[])
{
    sscanf(argv[1], "%d %d", &key_press_fd_read, &km_server_write);
}

void signal_handler(int signo, siginfo_t *siginfo, void *context) 
{
    // printf("Received signal number: %d \n", signo);
    if  (signo == SIGINT)
    {
        printf("Caught SIGINT \n");

        close(key_press_fd_read);
        close(km_server_write);
        
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
