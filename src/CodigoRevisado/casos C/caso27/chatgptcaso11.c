#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <libgen.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>

#define BUFFER_SIZE 4096
#define DEST_DIR "respaldo/"

int es_ruta_segura(const char *ruta) {
    return ruta[0] == '/' && strstr(ruta, "..") == NULL;
}

int es_archivo_regular(const char *ruta) {
    struct stat st;
    if (lstat(ruta, &st) != 0) return 0;
    return S_ISREG(st.st_mode);
}

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
        if (fwrite(buffer, 1, bytes, destino) != bytes) {
            perror("Error escribiendo archivo");
            fclose(origen);
            fclose(destino);
            exit(EXIT_FAILURE);
        }
    }

    fclose(origen);
    fclose(destino);
}

int main() {
    char ruta[PATH_MAX];

    printf("Ingrese la ruta absoluta del archivo a respaldar:\n");
    if (!fgets(ruta, sizeof(ruta), stdin)) {
        fprintf(stderr, "Error al leer la entrada.\n");
        return EXIT_FAILURE;
    }

    ruta[strcspn(ruta, "\n")] = '\0'; // Eliminar salto de línea

    if (!es_ruta_segura(ruta)) {
        fprintf(stderr, "Ruta no válida o potencialmente peligrosa.\n");
        return EXIT_FAILURE;
    }

    if (!es_archivo_regular(ruta)) {
        fprintf(stderr, "No es un archivo regular o no existe.\n");
        return EXIT_FAILURE;
    }

    struct stat st = {0};
    if (stat(DEST_DIR, &st) == -1) {
        if (mkdir(DEST_DIR, 0700) != 0) {
            perror("No se pudo crear la carpeta respaldo/");
            return EXIT_FAILURE;
        }
    }

    char ruta_absoluta[PATH_MAX];
    if (!realpath(ruta, ruta_absoluta)) {
        perror("Error al resolver la ruta absoluta");
        return EXIT_FAILURE;
    }

    char ruta_destino[PATH_MAX];
    snprintf(ruta_destino, sizeof(ruta_destino), "%s%s", DEST_DIR, basename(ruta_absoluta));

    copiar_archivo(ruta_absoluta, ruta_destino);

    printf("Archivo copiado exitosamente a %s\n", ruta_destino);
    return EXIT_SUCCESS;
}
