#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#define BUFFER_SIZE 4096
#define BACKUP_DIR "respaldo/"

int main() {
    char filepath[1024];
    char backup_path[1024];
    FILE *src_file, *dest_file;
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    char *filename;

    // Leer la ruta del archivo desde la entrada estándar
    printf("Ingrese la ruta absoluta del archivo a respaldar: ");
    if (fgets(filepath, sizeof(filepath), stdin) == NULL) {
        perror("Error al leer la entrada");
        return 1;
    }

    // Eliminar el salto de línea al final
    filepath[strcspn(filepath, "\n")] = '\0';

    // Verificar si la ruta es absoluta
    if (filepath[0] != '/') {
        fprintf(stderr, "Error: La ruta debe ser absoluta (comenzar con '/')\n");
        return 1;
    }

    // Crear el directorio respaldo si no existe
    if (mkdir(BACKUP_DIR, 0755) == -1) {
        if (errno != EEXIST) {
            perror("Error al crear el directorio respaldo");
            return 1;
        }
    }

    // Extraer el nombre del archivo de la ruta
    filename = strrchr(filepath, '/');
    if (filename == NULL) {
        fprintf(stderr, "Error: Ruta inválida\n");
        return 1;
    }
    filename++; // Saltar el '/' mismo

    // Construir la ruta de destino
    snprintf(backup_path, sizeof(backup_path), "%s%s", BACKUP_DIR, filename);

    // Abrir el archivo origen
    src_file = fopen(filepath, "rb");
    if (src_file == NULL) {
        perror("Error al abrir el archivo origen");
        return 1;
    }

    // Abrir el archivo destino
    dest_file = fopen(backup_path, "wb");
    if (dest_file == NULL) {
        perror("Error al abrir el archivo destino");
        fclose(src_file);
        return 1;
    }

    // Copiar el contenido
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, src_file)) > 0) {
        if (fwrite(buffer, 1, bytes_read, dest_file) != bytes_read) {
            perror("Error al escribir en el archivo destino");
            fclose(src_file);
            fclose(dest_file);
            return 1;
        }
    }

    // Cerrar los archivos
    fclose(src_file);
    fclose(dest_file);

    printf("Archivo copiado exitosamente a %s\n", backup_path);

    return 0;
}