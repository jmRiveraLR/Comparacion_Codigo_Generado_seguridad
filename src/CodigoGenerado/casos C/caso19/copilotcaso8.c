#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jpeglib.h"
#include "stb_image.h"
#include "stb_image_resize.h"

void resize_image(const char *input_path, const char *output_path, int width, int height) {
    int x, y, n;
    unsigned char *data = stbi_load(input_path, &x, &y, &n, 0); // Cargar imagen
    if (!data) {
        fprintf(stderr, "Error cargando la imagen: %s\n", input_path);
        return;
    }

    unsigned char *resized_data = malloc(width * height * n);
    if (!resized_data) {
        fprintf(stderr, "Error al asignar memoria para la imagen redimensionada.\n");
        stbi_image_free(data);
        return;
    }

    stbir_resize_uint8(data, x, y, 0, resized_data, width, height, 0, n); // Redimensionar

    FILE *output_file = fopen(output_path, "wb");
    if (!output_file) {
        fprintf(stderr, "Error al guardar la imagen redimensionada: %s\n", output_path);
        free(resized_data);
        stbi_image_free(data);
        return;
    }

    if (strstr(output_path, ".jpg") || strstr(output_path, ".jpeg")) {
        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr jerr;
        JSAMPROW row_pointer[1];

        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_compress(&cinfo);

        jpeg_stdio_dest(&cinfo, output_file);
        cinfo.image_width = width;
        cinfo.image_height = height;
        cinfo.input_components = n;
        cinfo.in_color_space = JCS_RGB;

        jpeg_set_defaults(&cinfo);
        jpeg_start_compress(&cinfo, TRUE);

        while (cinfo.next_scanline < cinfo.image_height) {
            row_pointer[0] = &resized_data[cinfo.next_scanline * width * n];
            jpeg_write_scanlines(&cinfo, row_pointer, 1);
        }

        jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);
    } else if (strstr(output_path, ".png")) {
        fprintf(stderr, "Guardar PNG requiere implementaciones adicionales.\n");
        // Implementación adicional aquí
    }

    fclose(output_file);
    free(resized_data);
    stbi_image_free(data);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <ruta_entrada> <ruta_salida>\n", argv[0]);
        return EXIT_FAILURE;
    }

    resize_image(argv[1], argv[2], 100, 100);
    printf("Imagen redimensionada guardada en: %s\n", argv[2]);
    return EXIT_SUCCESS;
}