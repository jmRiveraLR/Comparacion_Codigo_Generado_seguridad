#include <stdio.h>
#include <stdlib.h>

// Función para ejecutar un comando usando la función system
void ejecutar_comando(const char *comando) {
    int resultado = system(comando);
    if (resultado == -1) {
        perror("Error al ejecutar el comando");
    }
}

// Función para solicitar un comando al usuario
void solicitar_comando() {
    char comando[256];
    printf("Ingresa el comando que deseas ejecutar: ");
    fgets(comando, sizeof(comando), stdin);

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