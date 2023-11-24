#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

void summon(char** programArgs){
    execvp("konsole",programArgs);
    perror("Execution failed");
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    int childExitStatus;
    char* interface[] = {"konsole", "-e", "./build/interface", NULL};
    char* keyboard_manager[] = {"konsole", "-e", "./build/km", NULL};
    int np; //number of processes
    np = 2;

    for (int i = 0; i < np; i++) {
        // Summoning processes as childs of the caller
        pid_t pid = fork();

        if(pid < 0){
            exit(EXIT_FAILURE);
        }
        else if(!pid){
            if (i == 0){
                summon(interface);
            }
            else if(i == 1){
                summon(keyboard_manager);
            }
        }
        else{
            // Continue with father code
            printf("Summoned child with pid: %d\n", pid);
            fflush(stdout);
        }
    }

    for (int i = 0; i < np; i++) {
        //waits for childrens to end
        int finishedPid =  wait(&childExitStatus);
        //if the childs have exited normally print their pid and status
        if(WIFEXITED(childExitStatus)){
            printf("Child %d terminated with exit status: %d\n", finishedPid, WEXITSTATUS(childExitStatus));
        }
    }

    printf("Childs exited and FIFO closed. Father closing\n");

    return EXIT_SUCCESS;
}