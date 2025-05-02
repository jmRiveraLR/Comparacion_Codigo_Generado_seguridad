#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "jpeglib.h"
#include "stb_image.h"
#include "stb_image_resize.h"

#define OUTPUT_PERMISSIONS 0644 // Permisos para los archivos de salida

int is_valid_image(const char *path) {
    const char *ext = strrchr(path, '.');
    return ext && (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0 || strcasecmp(ext, ".png") == 0);
}

void resize_image(const char *input_path, const char *output_path, int width, int height) {
    if (!is_valid_image(input_path)) {
        fprintf(stderr, "El archivo no es una imagen válida: %s\n", input_path);
        return;
    }

    int x, y, n;
    unsigned char *data = stbi_load(input_path, &x, &y, &n, 0); // Cargar imagen
    if (!data) {
        fprintf(stderr, "Error cargando la imagen: %s\n", input_path);
        return;
    }

    if (n != 3 && n != 4) {
        fprintf(stderr, "Formato de imagen no soportado: %s\n", input_path);
        stbi_image_free(data);
        return;
    }

    unsigned char *resized_data = malloc(width * height * n);
    if (!resized_data) {
        fprintf(stderr, "Error al asignar memoria para la imagen redimensionada.\n");
        stbi_image_free(data);
        return;
    }

    stbir_resize_uint8(data, x, y, 0, resized_data, width, height, 0, n); // Redimensionar

    int fd = open(output_path, O_CREAT | O_WRONLY | O_TRUNC, OUTPUT_PERMISSIONS);
    if (fd == -1) {
        fprintf(stderr, "Error al abrir el archivo de salida: %s\n", output_path);
        free(resized_data);
        stbi_image_free(data);
        return;
    }

    FILE *output_file = fdopen(fd, "wb");
    if (!output_file) {
        fprintf(stderr, "Error al asociar el descriptor de archivo: %s\n", output_path);
        close(fd);
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
        fprintf(stderr, "Soporte para PNG pendiente de implementación.\n");
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

    if (!is_valid_image(argv[1])) {
        fprintf(stderr, "El archivo de entrada no es válido o no es una imagen soportada.\n");
        return EXIT_FAILURE;
    }

    resize_image(argv[1], argv[2], 100, 100);
    printf("Imagen redimensionada guardada en: %s\n", argv[2]);
    return EXIT_SUCCESS;
}