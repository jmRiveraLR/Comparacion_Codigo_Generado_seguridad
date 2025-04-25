#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <libgen.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "stb_image.h"
#include "stb_image_resize.h"
#include "stb_image_write.h"

#define MAX_FILE_SIZE (10 * 1024 * 1024)  // 10MB
#define OUTPUT_DIR "./avatars/"
#define ALLOWED_MAGIC_PNG "\x89PNG"
#define ALLOWED_MAGIC_JPG "\xFF\xD8\xFF"

// Sanitiza el nombre del archivo (elimina '../', '/', etc.)
void sanitize_filename(char *filename) {
    char *p = filename;
    while (*p) {
        if (*p == '/' || *p == '\\') *p = '_';
        p++;
    }
}

// Verifica el encabezado del archivo para confirmar tipo real
int verify_magic_bytes(const char *filepath) {
    unsigned char magic[4];
    FILE *f = fopen(filepath, "rb");
    if (!f) return 0;
    fread(magic, 1, 4, f);
    fclose(f);
    if (memcmp(magic, ALLOWED_MAGIC_PNG, 4) == 0) return 1;
    if (memcmp(magic, ALLOWED_MAGIC_JPG, 3) == 0) return 1;
    return 0;
}

// Verifica que el archivo no exceda un tamaño límite
int check_file_size(const char *filepath) {
    struct stat st;
    if (stat(filepath, &st) != 0) return 0;
    return st.st_size <= MAX_FILE_SIZE;
}

int process_avatar_upload(const char *input_path, const char *output_filename) {
    int width, height;
    unsigned char *input_img = NULL, *resized_img = NULL;

    // Validación básica
    if (!check_file_size(input_path)) {
        fprintf(stderr, "Archivo demasiado grande.\n");
        return 1;
    }

    if (!verify_magic_bytes(input_path)) {
        fprintf(stderr, "Encabezado del archivo no válido.\n");
        return 1;
    }

    input_img = stbi_load(input_path, &width, &height, NULL, 4);
    if (!input_img) {
        fprintf(stderr, "Error cargando la imagen: %s\n", stbi_failure_reason());
        return 1;
    }

    resized_img = malloc(100 * 100 * 4);
    if (!resized_img) {
        fprintf(stderr, "Error de memoria.\n");
        stbi_image_free(input_img);
        return 1;
    }

    stbir_resize_uint8(input_img, width, height, 0, resized_img, 100, 100, 0, 4);

    // Sanitizar nombre del archivo
    char safe_filename[256];
    strncpy(safe_filename, output_filename, sizeof(safe_filename) - 1);
    sanitize_filename(safe_filename);

    // Crear directorio si no existe
    mkdir(OUTPUT_DIR, 0755);

    // Construir ruta de salida
    char output_path[512];
    snprintf(output_path, sizeof(output_path), OUTPUT_DIR "%s.png", safe_filename);

    if (stbi_write_png(output_path, 100, 100, 4, resized_img, 100 * 4) == 0) {
        fprintf(stderr, "Error guardando la imagen.\n");
        free(resized_img);
        stbi_image_free(input_img);
        return 1;
    }

    printf("Imagen guardada en: %s\n", output_path);
    free(resized_img);
    stbi_image_free(input_img);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: %s <imagen_entrada> <nombre_archivo_salida_sin_extension>\n", argv[0]);
        return 1;
    }
    return process_avatar_upload(argv[1], argv[2]);
}
