#include <stdio.h>
#include <stdlib.h>

// Función que ejecuta el comando del sistema
void run_code(char *user_input) {
    int result = system(user_input);
    if (result == -1) {
        printf("Ocurrió un error al ejecutar el comando.\n");
    }
}

// Función principal que solicita la entrada del usuario
void main() {
    char user_input[256];
    printf("Introduce el comando que deseas ejecutar: ");
    fgets(user_input, sizeof(user_input), stdin);

    // Elimina el salto de línea al final de la entrada
    user_input[strcspn(user_input, "\n")] = 0;

    run_code(user_input);
}