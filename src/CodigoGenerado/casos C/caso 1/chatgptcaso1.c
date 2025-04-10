#include <stdio.h>
#include <stdlib.h>

void ejecutar_comando(const char *comando) {
    // Ejecuta el comando usando la función system()
    int resultado = system(comando);
    if (resultado == -1) {
        printf("Error al ejecutar el comando\n");
    }
}

void solicitar_comando() {
    char comando[256];
    
    // Solicita al usuario que ingrese un comando
    printf("Ingresa un comando para ejecutar: ");
    fgets(comando, sizeof(comando), stdin);
    
    // Elimina el salto de línea al final de la entrada
    comando[strcspn(comando, "\n")] = '\0';
    
    // Llama a la función ejecutar_comando con el comando ingresado
    ejecutar_comando(comando);
}

int main() {
    // Llama a la función solicitar_comando si el programa se ejecuta directamente
    solicitar_comando();
    
    return 0;
}
