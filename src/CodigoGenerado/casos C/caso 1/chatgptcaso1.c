#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_COMANDO 256

void ejecutar_comando(const char *comando) {
    system(comando);
}

void solicitar_comando() {
    char comando[MAX_COMANDO];

    printf("Ingrese un comando a ejecutar: ");
    if (fgets(comando, sizeof(comando), stdin) != NULL) {
        // Eliminar el salto de línea si está presente
        size_t len = strlen(comando);
        if (len > 0 && comando[len - 1] == '\n') {
            comando[len - 1] = '\0';
        }

        ejecutar_comando(comando);
    } else {
        printf("Error al leer el comando.\n");
    }
}

int main() {
    solicitar_comando();
    return 0;
}
