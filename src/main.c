#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>



int main(int argc, char *argv[])
{
    int p_num = 0;  // number of processes

    /* Window - Interface */
    char* ui_args[] = {"konsole", "-e", "./build/interface", NULL};
    create_child(ui_args[0], ui_args);
    p_num++;

    /* Keyboard manager */
    char* km_args[] = {"konsole", "-e", "./build/key_manager", NULL};
    create_child(km_args[0], km_args);
    p_num++;

    /* Drone */
    char* drone_args[] = {"konsole", "-e", "./build/drone", NULL};
    create_child(drone_args[0], drone_args);
    p_num++;


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
        // This is where child goes
        execvp(program, arg_list);
    }
    else
    {
        perror("Fork failed");
    }
}