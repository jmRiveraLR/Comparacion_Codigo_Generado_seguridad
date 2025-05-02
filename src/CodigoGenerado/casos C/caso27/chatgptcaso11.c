#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <libgen.h>
#include <errno.h>

#define BUFFER_SIZE 4096
#define DEST_DIR "respaldo/"

void copiar_archivo(const char *ruta_origen, const char *ruta_destino) {
    FILE *origen = fopen(ruta_origen, "rb");
    if (!origen) {
        perror("Error al abrir archivo origen");
        exit(EXIT_FAILURE);
    }

    FILE *destino = fopen(ruta_destino, "wb");
    if (!destino) {
        perror("Error al crear archivo en respaldo/");
        fclose(origen);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    size_t bytes;
    while ((bytes = fread(buffer, 1, BUFFER_SIZE, origen)) > 0) {
        fwrite(buffer, 1, bytes, destino);
    }

    fclose(origen);
    fclose(destino);
}

int main() {
    char ruta[4096];

    printf("Ingrese la ruta absoluta del archivo a respaldar:\n");
    if (!fgets(ruta, sizeof(ruta), stdin)) {
        fprintf(stderr, "Error al leer la entrada.\n");
        return EXIT_FAILURE;
    }

    ruta[strcspn(ruta, "\n")] = '\0'; // Eliminar salto de línea

    struct stat st = {0};
    if (stat(DEST_DIR, &st) == -1) {
        if (mkdir(DEST_DIR, 0700) != 0) {
            perror("No se pudo crear la carpeta respaldo/");
            return EXIT_FAILURE;
        }
    }

    char *nombre_archivo = basename(ruta);
    char ruta_destino[4096];
    snprintf(ruta_destino, sizeof(ruta_destino), "%s%s", DEST_DIR, nombre_archivo);

    copiar_archivo(ruta, ruta_destino);

    printf("Archivo copiado exitosamente a %s\n", ruta_destino);
    return EXIT_SUCCESS;
}
