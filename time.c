#include <stdio.h>      // Para funciones de entrada/salida como printf y perror
#include <stdlib.h>     // Para la función exit()
#include <sys/time.h>   // Para la estructura timeval y gettimeofday()
#include <unistd.h>     // Para funciones de manejo de procesos como fork() y execvp()
#include <sys/wait.h>   // Para la función wait() que espera a que el proceso hijo termine

int main(int argc, char *argv[]) {
    // Verificar si se recibió un comando como argumento
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command>\n", argv[0]); // Mensaje de uso
        return 1; // Salir con un código de error
    }

    struct timeval start, end; // Estructuras para almacenar tiempos inicial y final
    pid_t pid = fork(); // Crear un nuevo proceso

    if (pid < 0) { // Si fork() falla
        perror("Fork failed"); // Imprimir error
        return 1; // Salir con un código de error
    }

    if (pid == 0) { // Proceso hijo
        // Ejecutar el comando especificado en la línea de comandos
        execvp(argv[1], &argv[1]); // argv[1] es el comando, &argv[1] son los argumentos

        // Si execvp falla, imprimir error y salir del proceso hijo
        perror("Exec failed");
        exit(1); // Terminar el proceso hijo con un código de error
    } else { // Proceso padre
        gettimeofday(&start, NULL); // Obtener el tiempo inicial antes de ejecutar el comando
        wait(NULL); // Esperar a que el proceso hijo termine
        gettimeofday(&end, NULL);   // Obtener el tiempo final después de que el hijo termina

        // Calcular el tiempo transcurrido en segundos
        double elapsed = (end.tv_sec - start.tv_sec) + 
                         (end.tv_usec - start.tv_usec) / 1e6;

        // Imprimir el tiempo transcurrido en segundos con seis decimales de precisión
        printf("Elapsed time: %.6f seconds\n", elapsed);
    }

    return 0; // Salir del programa con éxito
}

