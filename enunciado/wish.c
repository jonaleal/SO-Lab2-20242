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
    if (args[0] == NULL)
    {
        return; // Empty command
    }

    // Built-in commands
    if (strcmp(args[0], "exit") == 0)
    {
        if (args[1] != NULL)
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
        if (args[2] != NULL)
        {
            printError();
        }
        else
        {
            chdir(args[1]);
        }
    }
    else if (strcmp(args[0], "path") == 0)
    {
        // Clear and update paths
        for (int i = 0; i < MAX_PATHS; i++)
            paths[i] = NULL;
        for (int i = 1; args[i] != NULL && i < MAX_PATHS; i++)
            if (args[i][0] == '/')
            {
                // Absolute path, add as is
                paths[i - 1] = args[i];
            }
            else
            {
                // Relative path, construct full path
                char *fullPath = malloc(strlen("./") + strlen(args[i]) + 1); // +1 for '\0'
                sprintf(fullPath, "./%s", args[i]);
                paths[i - 1] = fullPath;
            }
    }
    else
    {
        // Regular command execution
        pid_t pid = fork();
        if (pid == 0)
        {
            // In child process: Handle redirection if needed
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
            int found = 0; // Flag to check if command was found in any path
            for (int i = 0; paths[i] != NULL; i++)
            {
                snprintf(execPath, sizeof(execPath), "%s/%s", paths[i], args[0]);
                if (access(execPath, X_OK) == 0) // Check if the command exists and is executable
                {
                    execv(execPath, args); // Attempt to execute command
                    found = 1;             // Command found
                }
            }
            if (!found)
            {
                printError(); // If execv fails for all paths
            }
            exit(1);
        }
        else if (pid > 0)
        {
            // In parent process: Wait for child process to complete
            wait(NULL);
        }
        else
        {
            printError(); // Fork error
        }
    }
}

void parseAndExecute(char *line)
{
    char *command;
    char *saveptr; // Para mantener el estado entre llamadas a strtok_r
    char *args[MAX_ARGS];
    char *outputFile = NULL;
    int redirect = 0;
    
    // Separar los comandos por '&'
    command = strtok_r(line, "&", &saveptr);
    
    while (command != NULL)
    {
        char *token = strtok(command, " \t\n");
        int i = 0;

        // Verificar si el primer token es '>' sin un comando previo
        if (token != NULL && strcmp(token, ">") == 0)
        {
            printError();
            return;
        }

        // Parsear el comando y manejar redirección
        while (token != NULL && i < MAX_ARGS)
        {
            if (strcmp(token, ">") == 0)
            {
                redirect = 1;                  // Se detecta redirección
                token = strtok(NULL, " \t\n"); // El siguiente token debería ser el archivo de salida
                if (token == NULL || strtok(NULL, " \t\n") != NULL)
                {
                    // Error: sin archivo de salida o demasiados argumentos después de '>'
                    printError();
                    return;
                }
                outputFile = token;
                break;
            }
            args[i++] = token;
            token = strtok(NULL, " \t\n");
        }
        args[i] = NULL; // Null-terminar el array de argumentos

        // Ejecutar el comando actual
        executeCommand(args, outputFile, redirect);

        // Obtener el siguiente comando después del '&'
        command = strtok_r(NULL, "&", &saveptr);
    }
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