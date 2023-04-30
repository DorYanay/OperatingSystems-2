#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#define MAX_COMMAND_LENGTH 1024
#define MAX_COMMANDS 3
#define MAX_PIPES 2
#define MAX_COMMAND_ARGS 1024

int redirectFD(char **commands, int type);
void redirect(char **commands, int type);
int numberofcmds(char *commands[MAX_COMMANDS][MAX_COMMAND_ARGS]);
int redirectType(char **commands);
void execute(char **commands);
int FileIndex(char **commands);
void parse(char *commands[MAX_COMMANDS][MAX_COMMAND_ARGS], char *input);
volatile sig_atomic_t interrupted = 0;
// cntl-c
void sigint_handler(int sig)
{
}
void parse(char *commands[MAX_COMMANDS][MAX_COMMAND_ARGS], char *input)
{
    // check for pipes.
    commands[0][0] = strtok(input, "|");
    for (int i = 1; i < MAX_COMMANDS; i++)
    {
        commands[i][0] = strtok(NULL, "|");
    }
    // separate the commands.
    for (int i = 0; i < MAX_COMMANDS; i++)
    {
        commands[i][0] = strtok(commands[i][0], " ");
        for (int j = 1; j < MAX_COMMAND_ARGS; j++)
        {
            commands[i][j] = strtok(NULL, " ");
        }
    }
}

int numberofcmds(char *commands[MAX_COMMANDS][MAX_COMMAND_ARGS])
{
    int i = 0;
    for (; i < MAX_COMMANDS && commands[i][0] != NULL; i++)
    {
    }
    return i;
}

int FileIndex(char **commands)
{
    int i = 0;
    while (commands[i] != NULL)
    {
        i++;
    }
    return i;
}
void execute(char **commands)
{
    int type = redirectType(commands);
    if (type == 0)
    {
        execvp(commands[0], commands);
    }
    if (type == 1)
    {
        redirect(commands, 1);
    }
    if (type == 2)
    {
        redirect(commands, 2);
    }
}
void redirect(char **commands, int type)
{
    int index = FileIndex(commands);
    char *filename = commands[index - 1];
    commands[index - 1] = NULL;
    commands[index - 2] = NULL;
    if (type == 1)
    {
        int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0)
        {
            perror("open");
        }
        close(STDOUT_FILENO);
        if (dup2(fd, STDOUT_FILENO) == -1)
        {
            perror("dup2");
            exit(1);
        }
        close(fd);

        execvp(commands[0], commands);
    }
    else if (type == 2)
    {
        int fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd < 0)
        {
            perror("open");
        }
        close(STDOUT_FILENO);
        if (dup2(fd, STDOUT_FILENO) == -1)
        {
            perror("dup2");
            exit(1);
        }
        close(fd);

        execvp(commands[0], commands);
    }
}
int redirectFD(char **commands, int type)
{
    int index = FileIndex(commands);
    char *filename = commands[index - 1];
    commands[index - 1] = NULL;
    commands[index - 2] = NULL;
    if (type == 1)
    {
        int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0)
        {
            perror("open");
        }
        close(STDOUT_FILENO);
        if (dup2(fd, STDOUT_FILENO) == -1)
        {
            perror("dup2");
            exit(1);
        }

        return fd;
    }
    else if (type == 2)
    {
        int fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd < 0)
        {
            perror("open");
        }
        close(STDOUT_FILENO);
        if (dup2(fd, STDOUT_FILENO) == -1)
        {
            perror("dup2");
            exit(1);
        }

        return fd;
    }
    return 1;
}

int redirectType(char **commands)
{
    int i = 0;
    while (i < MAX_COMMAND_ARGS && commands[i] != NULL)
    {
        if (strcmp(commands[i], ">") == 0)
        {
            return 1;
        }
        if (strcmp(commands[i], ">>") == 0)
        {
            return 2;
        }
        i++;
    }
    return 0;
}

void execute_command(char ***commands, int num_pipes, int fdout)
{
    printf("%d-%s-%s-%s\n", num_pipes, commands[0][0], commands[1][0], commands[2][0]);

    pid_t pid1, pid2, pid3;

    int fd_pipes[2];
    int fd_pipes2[2];
    int out = fdout;

    if (pipe(fd_pipes) == -1)
    {
        perror("pipe");
        return;
    }

    pid1 = fork();
    if (pid1 < 0)
    {
        perror("fork");
        return;
    }
    else if (pid1 == 0)
    {
        dup2(fd_pipes[1], STDOUT_FILENO);
        close(fd_pipes[0]);
        close(fd_pipes[1]);
        execvp(commands[0][0], commands[0]);
        perror("execvp");
        return;
    }

    if (num_pipes > 1)
    {
        if (pipe(fd_pipes2) == -1)
        {
            perror("pipe");
            return;
        }

        out = fd_pipes2[1];
    }

    pid2 = fork();
    if (pid2 < 0)
    {
        perror("fork");
        return;
    }
    else if (pid2 == 0)
    {
        dup2(fd_pipes[0], STDIN_FILENO);
        dup2(out, STDOUT_FILENO);
        close(fd_pipes[0]);
        close(fd_pipes[1]);
        if (num_pipes > 1)
        {
            close(fd_pipes2[0]);
            close(fd_pipes2[1]);
        }
        execvp(commands[1][0], commands[1]);
        perror("execvp");
        return;
    }

    if (num_pipes == 1)
    {
        close(fd_pipes[0]);
        close(fd_pipes[1]);
        wait(0);
        wait(0);
        return;
    }

    pid3 = fork();
    if (pid3 < 0)
    {
        perror("fork");
        return;
    }
    else if (pid2 == 0)
    {
        dup2(fd_pipes2[0], STDIN_FILENO);
        dup2(fdout, STDOUT_FILENO);
        close(fd_pipes[0]);
        close(fd_pipes[1]);
        close(fd_pipes2[0]);
        close(fd_pipes2[1]);
        execvp(commands[2][0], commands[2]);
        perror("execvp");
        return;
    }

    close(fd_pipes[0]);
    close(fd_pipes[1]);
    close(fd_pipes2[0]);
    close(fd_pipes2[1]);
    wait(0);
    wait(0);
    wait(0);
}

int main()
{
    char input[MAX_COMMAND_LENGTH];
    char *commands[MAX_COMMANDS][MAX_COMMAND_ARGS] = {NULL};
    signal(SIGINT, sigint_handler);
    while (1)
    {
        printf("stshell > ");
        fflush(stdout);
        if (fgets(input, MAX_COMMAND_LENGTH, stdin) == NULL)
        {
            if (errno == EINTR && interrupted)
            {
                printf("^C\n");
                fflush(stdout);
                interrupted = 0;
                continue;
            }
        }
        input[strcspn(input, "\n")] = '\0';
        if (strcmp(input, "exit") == 0)
        {
            break;
        }
        // Parse command line
        parse(commands, input);
        int num_pipes = numberofcmds(commands) - 1;
        // Empty command
        if (commands[0][0] == NULL)
        {
            continue;
        }

        // Exit command
        if (strcmp(commands[0][0], "exit") == 0)
        {
            break;
        }
        // int num_cmds = num_pipes + 1;

        printf("%d-%s-%s-%s\n", num_pipes, commands[0][0], commands[1][0], commands[2][0]);

        if (num_pipes == 0)
        {
            if (fork() == 0)
            {
                execute(commands[0]);
            }
            else
            {
                wait(NULL);
            }
        }
        else
        {
            int fdout = redirectFD(commands[num_pipes], redirectType(commands[num_pipes]));
            execute_command((char ***)commands, num_pipes, fdout);
        }
    }
    return 0;
}
