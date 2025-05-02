#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Función para ejecutar un comando en el sistema
void ejecutar_comando(const char* comando) {
    printf("Ejecutando comando: %s\n", comando);
    int resultado = system(comando);
    
    if (resultado != 0) {
        printf("Error al ejecutar el comando (código %d)\n", resultado);
    }
}

// Función para solicitar un comando al usuario
void solicitar_comando() {
    char comando[256]; // Buffer para almacenar el comando
    
    printf("Por favor, ingrese un comando: ");
    
    // Leer la entrada del usuario
    if (fgets(comando, sizeof(comando), stdin) == NULL) {
        printf("Error al leer la entrada.\n");
        return;
    }
    
    // Eliminar el salto de línea final
    comando[strcspn(comando, "\n")] = '\0';
    
    // Ejecutar el comando
    ejecutar_comando(comando);
}

int main() {
    // Si el programa se ejecuta directamente, llamar a solicitar_comando
    solicitar_comando();
    return 0;
}