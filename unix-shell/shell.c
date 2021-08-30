/*
Zach Shim
CSS 430
P1: Unix Shell
Last Modified: January 22nd, 2021
*/

#include <stdio.h>
#include <stdlib.h>    //for exit
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#define MAX_LINE 80 /* The maximum length command */

/* Frees all memory in args from previous commands input by the user */
void freeArgs(char *args[], int index) {
    while(args[index] != NULL && index < MAX_LINE) {
        free(args[index]);
        args[index] = NULL;
        index++;
    }
}

/* History print function */
void printLastCommand(char *args[]) {
    printf("osh>");
    int index = 0;
    while(args[index] != NULL) {
        printf("%s", args[index]);
        index++;
    }
    printf("\n");
    fflush(stdout);
}

/* Return Values: Return 1 if there if valid commands;
                  Return 0 if there are invalid commands */
int parse(char *args[], int *concurrent, int *argsNum) {
    fflush(stdout);
    printf("osh>");
    fflush(stdout);
        
    // read input input into buffer
    char buf[256] = {};
    fgets(buf, 256, stdin);
    buf[strlen(buf) -1] = '\0';
    char* token = strtok(buf, " ");

    // check for special cases
    if(token == NULL){                      // \n new line input by user
        return 0;
    }
    
   if(!strcmp(token, "!!")) {         // HISTORY: repeat last system call
        if(args[0] == NULL) {
            printf("No Commands In History\n");
            return 0;
        }
        else {
            printLastCommand(args);
            return 1;
        }
    }

    // reset memory
    *concurrent = *argsNum = 0;
    freeArgs(args, 0);

    /* if token is argument then copy it to arguments list (args[]) */
    int iter = 0;
    while(token != NULL) {
        if(!strcmp(token, "exit")) {               // EXIT: exit current session
            printf("Exiting Session...\n");
            return -1;
        }
        else if(token[0] == '&') {                 // & OPERATOR: concurrency
            token = strtok(NULL, " ");
            if(token != NULL || *argsNum < 1) {    // error checking
                printf("Invalid Command\n");
                return 0;
            }
            *concurrent = 1;
            break;
        }
        else {
            args[iter] = strdup(token);
            iter++;
            *argsNum+=1;
            token = strtok(NULL, " ");
        }
    }

    return 1;
}

/* Child writes system call output to parent through pipe */
/* Parameters: args1 = first half arguments before operator;
               args2 = second half arguments after operator */
void runPipe(char *args1[], char *args2[]) {
    enum{READ, WRITE};
    int pipeFD[2];
    if(pipe(pipeFD) < 0) {
        perror("Error in creating pipe");
        exit(EXIT_FAILURE);
    }

    int status = 0;
    int pipe_pid = fork();

    if(pipe_pid < 0) {                             // error
        printf("Error In Process\n");
        printf("Aborting...");
        exit(EXIT_FAILURE);
    }
    else if(pipe_pid == 0) {                       // CHILD
        close(pipeFD[READ]);
        dup2(pipeFD[WRITE], STDOUT_FILENO);
        if(execvp(args1[0], args1) < 0) {          // writes to pipe
            freeArgs(args1, 0);
            freeArgs(args2, 0);
            printf("Error in execvp()\n");
            exit(EXIT_FAILURE);
        }
    }
    else {                                         // PARENT
        wait(NULL);
        close(pipeFD[WRITE]);
        dup2(pipeFD[READ], STDIN_FILENO);

        if(execvp(args2[0], args2) < 0) {          // read from pipe
            freeArgs(args1, 0);
            freeArgs(args2, 0);
            printf("Error in execvp()\n");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }
}

/* Parses and outputs Unix commands input by the user */
void runCommands() {
    char *args[MAX_LINE/2 + 1] = {};      /* command line arguments */
    int should_run = 1;                   /* flag to exit program */
    int concurrent = 0;                   /* flag for & from parse */
    int argsNum = 0;                      /* number of arguments */

    while (should_run) {
        int success = parse(args, &concurrent, &argsNum);
        
        // error and exit check
        if(success < 1) {
            if(success < 0) should_run = 0;
            continue;
        }

        int status = 0;
        pid_t pid;
        pid = fork();

        int usingPipe = 0;
        int commandIsValid = 1;
        if (pid < 0)                     // process failure
        {
            printf("Error In Process\n");
            printf("Aborting...");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)               // Child
        {
            for(int i = 0; i < argsNum; i++) {
                if(!(strcmp(args[i], "<"))) {
                    // error check
                    if(argsNum < 2 || args[i + 1] == NULL || i == 0) {
                        commandIsValid = 0;
                        break;
                    }
                    
                    // open file and dup fd
                    int fd = open(args[argsNum - 1], O_RDONLY);
                    if(fd == -1) {
                        printf("Invalid File Descriptor\n");
                        break;
                    }
                    freeArgs(args, i);
                    dup2(fd, STDIN_FILENO);
                    break;
                }
                else if(!(strcmp(args[i], ">"))) {
                    // error check
                    if(argsNum < 2 || args[i + 1] == NULL || i == 0) {
                        commandIsValid = 0;
                        break;
                    }

                    // open file and dup fd
                    int fd = open(args[argsNum - 1], O_CREAT | O_TRUNC | O_APPEND | O_WRONLY, 0666);
                    if(fd == -1) {
                        freeArgs(args, 0);
                        printf("Invalid File Descriptor\n");
                        break;
                    }
                    freeArgs(args, i);
                    dup2(fd, STDOUT_FILENO);
                    break;
                }
                else if(!(strcmp(args[i], "|"))) {
                    // error check
                    if(argsNum < 2 || args[i + 1] == NULL || i == 0) {
                        commandIsValid = 0;
                        break;
                    }

                    // copy arguments from right side of operator to own array
                    char *args2[MAX_LINE/2 + 1] = {};
                    int j = i+1, k = 0;
                    while(args[j] != NULL) {
                        args2[k] = strdup(args[j]);
                        j++;
                        k++;
                    }
                    freeArgs(args, i);
                    
                    usingPipe = 1;
                    runPipe(args, args2);
                }
            } // end for loop
            
            if(usingPipe == 0 && commandIsValid == 1) {
                if (execvp(args[0], args) < 0) {
                    freeArgs(args, 0);
                    printf("Error in execvp()\n");
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_SUCCESS);
            }
            else if(commandIsValid == 0) {
                freeArgs(args, 0);
                printf("Invalid Command\n");
                exit(EXIT_FAILURE);
            }
        }
        else                       // parent
        {
            if(concurrent == 0) wait(&status);
        }
    }
}
int main(void)
{
    runCommands();
    return 0;
}
