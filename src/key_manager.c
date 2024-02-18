#include "key_manager.h"
#include "common.h"

// PIPES: File descriptor arrays
int interface_km;
int km_server;

int main(int argc, char *argv[]) 
{
    // Read the file descriptors from the arguments
    get_args(argc, argv);

    // Signals
    struct sigaction sa;
    sa.sa_sigaction = signal_handler; 
    sa.sa_flags = SA_SIGINFO;
    sigaction (SIGINT, &sa, NULL);
    sigaction (SIGUSR1, &sa, NULL);   
    publish_pid_to_wd(KM_SYM, getpid());


    while (1)
    {
        //////////////////////////////////////////////////////
        /* SECTION 1: READ DATA FROM INTERFACE */
        /////////////////////////////////////////////////////

        char interface_msg[MSG_LEN];
        ssize_t bytes_read = read(interface_km, interface_msg, MSG_LEN);
        char pressed_key = interface_msg[0];
        printf("Pressed key: %c\n", pressed_key);
        // log_msg(msg_pressed_key);


        //////////////////////////////////////////////////////
        /* SECTION 2: DRONE ACTION SELECTION */
        /////////////////////////////////////////////////////
        
        char *action = determine_action(pressed_key);

        if (action != "None"){
            write_to_pipe(km_server, action);
            printf("[PIPE] SENT to server.c: %s\n", action);
            // log_msg(msg);
        }
    }

    return 0;
}


char* determine_action(char pressed_key)
{
    char key = toupper(pressed_key);

    /* Returns are in the format "x,y" */ 

    // Disclaimer: Y axis is inverted on tested terminal.
    if (key == 'W'){return "0,-1";}
    if (key == 'X'){return "0,1";}
    if (key == 'A'){return "-1,0";}
    if (key == 'D'){return "1,0";}
    if (key == 'Q'){return "-1,-1";}
    if (key == 'E'){return "1,-1";}
    if (key == 'Z'){return "-1,1";}
    if (key == 'C'){return "1,1";}
    if (key == 'S'){return "10,0";}  // Special value interpreted by drone.c process
    if (key == 'P'){return "STOP";}  // Special value interpreted by server.c process
    else {return "None";}
}


void signal_handler(int signo, siginfo_t *siginfo, void *context) 
{
    // printf("Received signal number: %d \n", signo);
    if  (signo == SIGINT)
    {
        printf("Caught SIGINT \n");
        close(interface_km);
        close(km_server);
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


void log_msg(char *msg)
{
    write_message_to_logger(KM_SYM, INFO, msg);
}


void get_args(int argc, char *argv[])
{
    sscanf(argv[1], "%d %d", &interface_km, &km_server);
}
