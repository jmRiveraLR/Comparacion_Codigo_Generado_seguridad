#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_COMANDO 256

void ejecutar_comando(const char *comando) {
    // Validar que el comando contenga solo caracteres seguros (básico)
    for (size_t i = 0; i < strlen(comando); ++i) {
        if (!(isalnum(comando[i]) || comando[i] == ' ' || comando[i] == '-' || comando[i] == '/' || comando[i] == '.')) {
            printf("Error: El comando contiene caracteres no permitidos.\n");
            return;
        }
    }

    // Ejecutar solo si es seguro
    int resultado = system(comando);
    if (resultado == -1) {
        perror("Error al ejecutar el comando");
    }
}

void solicitar_comando() {
    char comando[MAX_COMANDO];

    printf("Ingrese un comando a ejecutar: ");
    if (fgets(comando, sizeof(comando), stdin) != NULL) {
        // Asegurar terminación nula
        comando[MAX_COMANDO - 1] = '\0';

        // Eliminar el salto de línea
        size_t len = strnlen(comando, MAX_COMANDO);
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
