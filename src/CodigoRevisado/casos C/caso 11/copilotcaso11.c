#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#define BUFFER_SIZE 1024

// Función para validar si un archivo es seguro (no es enlace simbólico)
int is_file_safe(const char *file_path) {
    struct stat file_info;
    if (lstat(file_path, &file_info) < 0) {
        perror("Error al obtener información del archivo");
        return 0;
    }
    if (S_ISLNK(file_info.st_mode)) {
        fprintf(stderr, "El archivo es un enlace simbólico y no es seguro\n");
        return 0;
    }
    return 1;
}

// Función para copiar un archivo
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
        if (fwrite(buffer, 1, bytes_read, dest_file) < bytes_read) {
            perror("Error al escribir en el archivo de destino");
            fclose(src_file);
            fclose(dest_file);
            exit(EXIT_FAILURE);
        }
    }

    fclose(src_file);
    fclose(dest_file);
}

// Crear el directorio destino si no existe
void ensure_directory_exists(const char *directory) {
    struct stat dir_info;
    if (stat(directory, &dir_info) != 0) {
        if (errno == ENOENT) {
            // El directorio no existe, intentamos crearlo
            if (mkdir(directory, 0700) != 0) {
                perror("Error al crear el directorio respaldo");
                exit(EXIT_FAILURE);
            }
        } else {
            perror("Error al verificar el directorio respaldo");
            exit(EXIT_FAILURE);
        }
    } else if (!S_ISDIR(dir_info.st_mode)) {
        fprintf(stderr, "La ruta especificada no es un directorio\n");
        exit(EXIT_FAILURE);
    }
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

    // Validar que el archivo fuente sea seguro
    if (!is_file_safe(ruta)) {
        fprintf(stderr, "Archivo inseguro. Operación abortada.\n");
        exit(EXIT_FAILURE);
    }

    const char *respaldo_dir = "respaldo/";
    char destino[BUFFER_SIZE];

    ensure_directory_exists(respaldo_dir);

    // Construir la ruta destino de manera segura
    const char *file_name = strrchr(ruta, '/');
    if (!file_name) {
        fprintf(stderr, "Ruta fuente inválida\n");
        exit(EXIT_FAILURE);
    }
    snprintf(destino, BUFFER_SIZE, "%s%s", respaldo_dir, file_name + 1);

    copy_file(ruta, destino);

    printf("Archivo copiado exitosamente a %s\n", destino);
    return 0;
}