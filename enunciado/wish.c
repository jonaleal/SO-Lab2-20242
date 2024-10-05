#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define MAX_INPUT 1024
#define MAX_PATHS 10

// Inicializar la lista de rutas con solo el directorio /bin
char *path_list[MAX_PATHS] = {"/bin", NULL}; 

// Función para ejecutar comandos ingresados por el usuario
void execute_command(char *command) {
    char *args[MAX_INPUT]; // Array para almacenar los argumentos del comando
    char *token; // Variable para el token de comando
    int i = 0;

    // Parsear el comando ingresado en tokens (argumentos)
    token = strtok(command, " \n"); // Dividir por espacios y nueva línea
    while (token != NULL) {
        args[i++] = token; // Almacenar cada token en el array de argumentos
        token = strtok(NULL, " \n"); // Continuar dividiendo
    }
    args[i] = NULL; // Terminar la lista de argumentos con NULL

    // Comando integrado: exit
    if (strcmp(args[0], "exit") == 0) {
        if (i > 1) { // Si hay más de un argumento, es un error
            fprintf(stderr, "An error has occurred\n");
        } else {
            exit(0); // Salir del shell si no hay argumentos
        }
        return; // Volver al prompt sin ejecutar más comandos
    }

    // Comando integrado: cd
    if (strcmp(args[0], "cd") == 0) {
        if (i != 2) { // cd debe tener solo un argumento
            fprintf(stderr, "An error has occurred\n");
        } else {
            if (chdir(args[1]) != 0) { // Cambiar de directorio
                perror("Error al cambiar de directorio");
            }
        }
        return; // Volver al prompt sin ejecutar más comandos
    }

    // Comando integrado: path
    if (strcmp(args[0], "path") == 0) {
        int j;
        for (j = 0; j < MAX_PATHS; j++) {
            path_list[j] = NULL; // Limpiar la lista de rutas antes de agregar nuevas
        }
        for (j = 1; j < i; j++) {
            path_list[j - 1] = args[j]; // Agregar nuevas rutas a la lista
        }
        path_list[j - 1] = NULL; // Terminar la lista con NULL
        return; // Volver al prompt sin ejecutar más comandos
    }

    // Manejo de redirección de salida con el símbolo '>'
    int redirect_output = -1; // Variable para identificar si hay redirección
    for (int j = 0; args[j] != NULL; j++) {
        if (strcmp(args[j], ">") == 0) {
            redirect_output = j; // Guardar la posición del operador de redirección
            args[j] = NULL; // Terminar los argumentos aquí para el comando ejecutable
            break;
        }
    }

    pid_t pid = fork(); // Crear un nuevo proceso
    if (pid == 0) { // Proceso hijo
        if (redirect_output != -1) { // Si hay redirección de salida
            int fd = open(args[redirect_output + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) { // Verificar si se pudo abrir el archivo para redirigir la salida
                perror("Error al abrir el archivo de salida");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO); // Redirigir la salida estándar al archivo
            close(fd); // Cerrar el descriptor del archivo después de redirigir
        }

        // Intentar ejecutar el comando en las rutas especificadas por path_list
        char command_path[MAX_INPUT]; 
        for (int j = 0; path_list[j] != NULL; j++) {
            snprintf(command_path, sizeof(command_path), "%s/%s", path_list[j], args[0]);
            execv(command_path, args); // Ejecutar el comando encontrado en la ruta
        }

        fprintf(stderr, "An error has occurred\n"); // Mensaje de error si execv falla
        exit(1); // Salir del proceso hijo en caso de error
    } else if (pid > 0) { // Proceso padre
        wait(NULL); // Esperar a que termine el proceso hijo antes de continuar
    } else {
        perror("Error al crear el proceso"); // Mensaje de error si fork falla
    }
}

int main(int argc, char *argv[]) {
    char *input = NULL; 
    size_t len = 0;

    // Modo batch: leer desde un archivo si se proporciona un argumento al ejecutar el shell
    if (argc > 1) {
        FILE *file = fopen(argv[1], "r"); 
        if (!file) { 
            perror("Error al abrir el archivo");
            exit(1);
        }
        
        while (getline(&input, &len, file) != -1) { 
            execute_command(input); // Ejecutar cada línea como un comando 
        }
        
        fclose(file); 
        free(input); 
        return 0;
    }

    // Modo interactivo: bucle que solicita comandos al usuario continuamente
    while (1) {
        printf("wish> "); 
        
        if (getline(&input, &len, stdin) == -1) { 
            free(input); 
            exit(0); // Salir en caso de EOF 
        }

        execute_command(input); // Ejecutar el comando ingresado por el usuario 
    }

    free(input); 
    return 0;
}

