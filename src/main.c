#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <sys/wait.h>
#include <sys/types.h>


// Pipes
int key_pressing_pfd[2];

// TODO: Execute the new files (targets.c and obstacles.c)
// TODO: Create all the required pipes.
// TODO: Only execute in Konsole the interface.c and the drone.c for monitoring of force, pos, vel.


int main(int argc, char *argv[])
{
    // PIDs
    pid_t server_pid;
    pid_t window_pid;
    pid_t km_pid;
    pid_t drone_pid;
    pid_t wd_pid;

    create_pipes();

    // File descriptors for  given files
    char key_manager_fds[80];
    char window_fds[80];

    sprintf(key_manager_fds, "%d %d", key_pressing_pfd[0], key_pressing_pfd[1]);
    sprintf(window_fds, "%d %d", key_pressing_pfd[0], key_pressing_pfd[1]);


    int delay = 100000; // Time delay between next spawns
    int p_num = 0;  // number of processes


    /* Server */
    char* server_args[] = {"konsole", "-e", "./build/server", NULL};
    server_pid = create_child(server_args[0], server_args);
    p_num++;
    usleep(delay*10); // little bit more time for server

    /* Keyboard manager */
    char* km_args[] = {"konsole", "-e", "./build/key_manager", key_manager_fds, NULL};
    km_pid = create_child(km_args[0], km_args);
    p_num++;
    usleep(delay);

    /* Drone */
    char* drone_args[] = {"konsole", "-e", "./build/drone", NULL};
    drone_pid = create_child(drone_args[0], drone_args);
    p_num++;
    usleep(delay);

    /* Watchdog */
    char* wd_args[] = {"konsole", "-e", "./build/watchdog", NULL};
    wd_pid = create_child(wd_args[0], wd_args);
    p_num++;
    printf("Watchdog Created\n");

    /* Window - Interface */
    char* window_args[] = {"konsole", "-e", "./build/interface", window_fds, NULL};
    window_pid = create_child(window_args[0], window_args);
    p_num++;
    usleep(delay);

    /* Wait for all children to close */
    for (int i = 0; i < p_num; i++)
    {
        int pid, status;
        pid = wait(&status);

        if (WIFEXITED(status))
        {
            printf("Child process with PID: %i terminated with exit status: %i\n", pid, WEXITSTATUS(status));
        }
    }
    printf("All child processes closed, closing main process...\n");
    return 0;
}

void create_pipes()
{
    pipe(key_pressing_pfd);

    printf("Pipes Succesfully created");
}

int create_child(const char *program, char **arg_list)
{
    pid_t child_pid = fork();
    if (child_pid != 0)
    {
        printf("Child process %s with PID: %d  was created\n", program, child_pid);
        return child_pid;
    }
    else if (child_pid == 0)
    {   
        execvp(program, arg_list);
    }
    else
    {
        perror("Fork failed");
    }
}