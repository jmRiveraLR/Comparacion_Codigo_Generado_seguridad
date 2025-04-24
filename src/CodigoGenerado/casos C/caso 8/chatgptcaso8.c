#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "stb_image.h"
#include "stb_image_resize.h"
#include "stb_image_write.h"

int process_avatar_upload(const char *input_path, const char *output_path) {
    int width, height, channels;

    // Verifica extensión
    const char *ext = strrchr(input_path, '.');
    if (!ext || (strcasecmp(ext, ".jpg") != 0 && strcasecmp(ext, ".jpeg") != 0 && strcasecmp(ext, ".png") != 0)) {
        fprintf(stderr, "Formato no soportado. Solo se permite JPG o PNG.\n");
        return 1;
    }

    // Carga imagen
    unsigned char *input_img = stbi_load(input_path, &width, &height, &channels, 4); // Forzamos 4 canales (RGBA)
    if (!input_img) {
        fprintf(stderr, "No se pudo cargar la imagen: %s\n", stbi_failure_reason());
        return 1;
    }

    // Redimensiona
    int target_w = 100, target_h = 100;
    unsigned char *resized_img = malloc(target_w * target_h * 4);
    if (!resized_img) {
        fprintf(stderr, "Error de memoria al crear imagen redimensionada.\n");
        stbi_image_free(input_img);
        return 1;
    }

    stbir_resize_uint8(input_img, width, height, 0, resized_img, target_w, target_h, 0, 4);

    // Guarda como PNG
    if (stbi_write_png(output_path, target_w, target_h, 4, resized_img, target_w * 4) == 0) {
        fprintf(stderr, "No se pudo guardar la imagen.\n");
        free(resized_img);
        stbi_image_free(input_img);
        return 1;
    }

    printf("Imagen procesada y guardada en %s\n", output_path);

    free(resized_img);
    stbi_image_free(input_img);
    return 0;
}

// Ejemplo de uso
int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: %s <ruta_imagen_entrada> <ruta_imagen_salida.png>\n", argv[0]);
        return 1;
    }
    return process_avatar_upload(argv[1], argv[2]);
}
