#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>

// Función para validar si el comando es seguro (solo letras, números y espacios)
int es_comando_seguro(const char* comando) {
    for (size_t i = 0; i < strlen(comando); i++) {
        if (!isalnum(comando[i]) && comando[i] != ' ' && comando[i] != '-' && comando[i] != '/') {
            return 0; // Carácter no permitido
        }
    }
    return 1; // Válido
}

// Función para ejecutar comandos de forma segura (sin shell)
void ejecutar_comando_seguro(const char* comando) {
    if (!es_comando_seguro(comando)) {
        fprintf(stderr, "Error: Comando no permitido.\n");
        return;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return;
    } else if (pid == 0) { // Proceso hijo
        char* args[] = {"/bin/sh", "-c", (char*)comando, NULL};
        execvp(args[0], args);
        perror("execvp"); // Solo se ejecuta si falla execvp
        exit(EXIT_FAILURE);
    } else { // Proceso padre
        int status;
        waitpid(pid, &status, 0);
    }
}

// Función para solicitar comando con buffer dinámico (sin overflow)
void solicitar_comando_seguro() {
    char* comando = NULL;
    size_t tam = 0;
    ssize_t bytes_leidos;

    printf("Ingrese un comando seguro (solo letras, números y espacios): ");
    bytes_leidos = getline(&comando, &tam, stdin);

    if (bytes_leidos == -1) {
        perror("getline");
        free(comando);
        return;
    }

    // Eliminar salto de línea
    comando[strcspn(comando, "\n")] = '\0';

    ejecutar_comando_seguro(comando);
    free(comando); // Liberar memoria
}

int main() {
    solicitar_comando_seguro();
    return 0;
}