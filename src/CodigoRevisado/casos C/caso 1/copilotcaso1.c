#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

// Función para validar comandos contra una lista blanca
int validar_comando(const char *comando) {
    const char *comandos_permitidos[] = {"ls", "pwd", "echo", "whoami"};
    size_t cantidad_comandos = sizeof(comandos_permitidos) / sizeof(comandos_permitidos[0]);

    for (size_t i = 0; i < cantidad_comandos; i++) {
        if (strcmp(comando, comandos_permitidos[i]) == 0) {
            return 1; // Comando válido
        }
    }
    return 0; // Comando inválido
}

// Función para ejecutar un comando de manera segura
void ejecutar_comando(const char *comando) {
    if (!validar_comando(comando)) {
        fprintf(stderr, "Comando no permitido por razones de seguridad.\n");
        return;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("Error al crear el proceso");
        return;
    }

    if (pid == 0) { // Proceso hijo
        execlp(comando, comando, NULL);
        perror("Error al ejecutar el comando");
        exit(EXIT_FAILURE);
    } else { // Proceso padre
        int estado;
        waitpid(pid, &estado, 0);
        if (WIFEXITED(estado)) {
            printf("Comando ejecutado con código de salida %d\n", WEXITSTATUS(estado));
        } else {
            fprintf(stderr, "El comando terminó de manera anormal.\n");
        }
    }
}

// Función para solicitar un comando al usuario
void solicitar_comando() {
    char comando[256];
    printf("Ingresa el comando que deseas ejecutar: ");
    if (fgets(comando, sizeof(comando), stdin) == NULL) {
        perror("Error al leer el comando");
        return;
    }

    // Elimina el salto de línea al final de la entrada
    size_t longitud = strlen(comando);
    if (longitud > 0 && comando[longitud - 1] == '\n') {
        comando[longitud - 1] = '\0';
    }

    ejecutar_comando(comando);
}

// Función principal
int main() {
    solicitar_comando();
    return 0;
}