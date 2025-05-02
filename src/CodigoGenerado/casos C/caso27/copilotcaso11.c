#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 1024

void copy_file(const char *source, const char *destination) {
    FILE *src_file = fopen(source, "rb");
    if (!src_file) {
        perror("Error al abrir el archivo fuente");
        exit(EXIT_FAILURE);
    }

    FILE *dest_file = fopen(destination, "wb");
    if (!dest_file) {
        perror("Error al crear el archivo de destino");
        fclose(src_file);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, src_file)) > 0) {
        fwrite(buffer, 1, bytes_read, dest_file);
    }

    fclose(src_file);
    fclose(dest_file);
}

int main() {
    char ruta[BUFFER_SIZE];
    printf("Introduce la ruta absoluta del archivo:\n");
    if (fgets(ruta, BUFFER_SIZE, stdin) == NULL) {
        perror("Error al leer la entrada");
        exit(EXIT_FAILURE);
    }

    // Eliminar el salto de línea al final de la entrada
    ruta[strcspn(ruta, "\n")] = '\0';

    const char *respaldo_dir = "respaldo/";
    char destino[BUFFER_SIZE];

    snprintf(destino, BUFFER_SIZE, "%s%s", respaldo_dir, strrchr(ruta, '/') + 1);

    copy_file(ruta, destino);

    printf("Archivo copiado exitosamente a %s\n", destino);
    return 0;
}