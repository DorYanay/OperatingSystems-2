#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <fcntl.h>
#include "stdio.h"
#include "errno.h"
#include "stdlib.h"
#include "unistd.h"
#include <string.h>
#include <signal.h>
#define MAX_COMMAND_LENGTH 1024
volatile sig_atomic_t interrupted = 0;
// cntl-c
void sigint_handler(int sig)
{
    interrupted = 1;
}
int main()
{
    int i;
    char *argv[10];
    char command[MAX_COMMAND_LENGTH];
    char *token;
    signal(SIGINT, sigint_handler);
    while (1)
    {
        // argc = 0;
        interrupted = 0;
        printf("stshell > ");
        fflush(stdout);
        while (fgets(command, 1024, stdin) == NULL)
        {
            // ctrl-c handler
            if (errno == EINTR && interrupted)
            {
                printf("^C\n");
                fflush(stdout);
                interrupted = 0;
                break;
            }
        }
        command[strlen(command) - 1] = '\0';
        // "exit" handler
        if (strcmp(command, "exit") == 0)
        {
            break;
        }
        /* parse command line */
        i = 0;
        token = strtok(command, " ");
        while (token != NULL)
        {
            argv[i] = token;
            token = strtok(NULL, " ");
            i++;
        }
        argv[i] = NULL;

        /* Is command empty */
        if (argv[0] == NULL)
            continue;

        /* for commands not part of the shell command language */
        if (fork() == 0)
        {
            execvp(argv[0], argv);
            wait(NULL);
        }
    }
}
