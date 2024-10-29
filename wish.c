#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_PATHS 10
#define MAX_ARGS 10
#define ERROR_MESSAGE "An error has occurred\n"

// Path array
char *paths[MAX_PATHS] = {"/bin", NULL};

void printError()
{
    write(STDERR_FILENO, ERROR_MESSAGE, strlen(ERROR_MESSAGE));
}

void executeCommand(char **args, char *outputFile, int redirect)
{
    if (!args[0])
        return; // Empty command

    // Built-in commands
    if (strcmp(args[0], "exit") == 0)
    {
        if (args[1])
        {
            printError(); // exit takes no arguments
        }
        else
        {
            exit(0);
        }
    }
    else if (strcmp(args[0], "cd") == 0)
    {
        if (!args[1] || args[2])
        {
            printError();
        }
        else
        {
            if (chdir(args[1]) != 0)
                printError(); // Handle chdir failure
        }
    }
    else if (strcmp(args[0], "path") == 0)
    {
        memset(paths, 0, sizeof(paths)); // Clear paths
        for (int i = 1; args[i] && i < MAX_PATHS; i++)
        {
            paths[i - 1] = strdup(args[i]); // Directly copy paths
        }
    }
    else
    {
        // Regular command execution
        pid_t pid = fork();
        if (pid == 0)
        {
            // Child process: Handle redirection if needed
            if (redirect)
            {
                int fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
                if (fd < 0)
                {
                    printError();
                    exit(1);
                }
                dup2(fd, STDOUT_FILENO); // Redirect stdout to the file
                dup2(fd, STDERR_FILENO); // Redirect stderr to the file
                close(fd);
            }

            // Try to execute command from each path
            char execPath[100];
            for (int i = 0; paths[i]; i++)
            {
                snprintf(execPath, sizeof(execPath), "%s/%s", paths[i], args[0]);
                if (access(execPath, X_OK) == 0)
                {                          // Check if the command exists and is executable
                    execv(execPath, args); // Attempt to execute command
                }
            }
            printError(); // If execv fails for all paths
            exit(1);
        }
        else if (pid < 0)
        {
            printError(); // Fork error
        }
        // No wait(NULL) here; parent process continues to the next command
    }
}

void parseAndExecute(char *line)
{
    char *command, *saveptr, *args[MAX_ARGS], *outputFile = NULL;
    int redirect = 0;

    // Split commands by '&'
    command = strtok_r(line, "&", &saveptr);

    while (command)
    {
        char *token = strtok(command, " \t\n");
        int i = 0;

        if (token && strcmp(token, ">") == 0)
        {
            printError();
            return;
        }

        // Parse command and handle redirection
        while (token && i < MAX_ARGS)
        {
            if (strcmp(token, ">") == 0)
            {
                redirect = 1;                  // Redirection detected
                token = strtok(NULL, " \t\n"); // Next token should be output file
                if (!token || strtok(NULL, " \t\n"))
                {
                    printError(); // Error: no output file or too many args
                    return;
                }
                outputFile = token;
                break;
            }
            args[i++] = token;
            token = strtok(NULL, " \t\n");
        }
        args[i] = NULL; // Null-terminate the argument array

        // Execute the command in the current process
        executeCommand(args, outputFile, redirect);

        command = strtok_r(NULL, "&", &saveptr); // Get the next command
    }

    // Wait for all child processes to finish
    while (wait(NULL) > 0)
        ; // This will wait for all child processes
}

int main(int argc, char *argv[])
{
    FILE *input = stdin;
    if (argc == 2)
    {
        input = fopen(argv[1], "r");
        if (input == NULL)
        {
            printError();
            exit(1);
        }
    }
    else if (argc > 2)
    {
        printError();
        exit(1);
    }

    char *line = NULL;
    size_t len = 0;
    while (1)
    {
        if (argc == 1)
        {
            printf("wish> "); // Print prompt only in interactive mode
        }
        if (getline(&line, &len, input) == -1)
        {
            break; // EOF reached
        }
        parseAndExecute(line);
    }

    free(line);
    if (argc == 2)
    {
        fclose(input);
    }
    return 0;
}
