#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <jpeglib.h>

// Define tamaños máximos para validación
#define MAX_BMP_HEADER_SIZE 54
#define MAX_IMAGE_DIMENSION 10000 // Limitar dimensiones para evitar DoS

void validate_bmp_header(unsigned char *header) {
    // Validar dimensiones del archivo BMP
    int width = *(int *)&header[18];
    int height = *(int *)&header[22];

    if (width <= 0 || height <= 0 || width > MAX_IMAGE_DIMENSION || height > MAX_IMAGE_DIMENSION) {
        fprintf(stderr, "Dimensiones de la imagen inválidas o demasiado grandes.\n");
        exit(EXIT_FAILURE);
    }
}

void safe_open_file(const char *path, const char *mode, FILE **file) {
    // Verificar ruta del archivo y abrirlo de forma segura
    if (!path || strlen(path) == 0) {
        fprintf(stderr, "La ruta del archivo no puede ser nula o vacía.\n");
        exit(EXIT_FAILURE);
    }

    *file = fopen(path, mode);
    if (!(*file)) {
        fprintf(stderr, "Error al abrir archivo: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void convert_bmp_to_jpeg(const char *bmp_path, const char *jpeg_path) {
    FILE *bmp_file, *jpeg_file;

    // Abrir archivos de manera segura
    safe_open_file(bmp_path, "rb", &bmp_file);
    safe_open_file(jpeg_path, "wb", &jpeg_file);

    // Leer encabezado BMP con validación
    unsigned char bmp_header[MAX_BMP_HEADER_SIZE];
    fread(bmp_header, sizeof(unsigned char), MAX_BMP_HEADER_SIZE, bmp_file);
    validate_bmp_header(bmp_header);

    int width = *(int *)&bmp_header[18];
    int height = *(int *)&bmp_header[22];

    // Asignar memoria con límites
    unsigned char *bmp_data = malloc(3 * width * height);
    if (!bmp_data) {
        fprintf(stderr, "Error al asignar memoria para los datos BMP.\n");
        fclose(bmp_file);
        fclose(jpeg_file);
        exit(EXIT_FAILURE);
    }

    fread(bmp_data, sizeof(unsigned char), 3 * width * height, bmp_file);

    // Configuración de libjpeg
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    jpeg_stdio_dest(&cinfo, jpeg_file);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_start_compress(&cinfo, TRUE);

    unsigned char *row_pointer = bmp_data;
    while (cinfo.next_scanline < cinfo.image_height) {
        row_pointer = bmp_data + cinfo.next_scanline * width * 3;
        jpeg_write_scanlines(&cinfo, &row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    // Liberar memoria y cerrar archivos de manera segura
    free(bmp_data);
    fclose(bmp_file);
    fclose(jpeg_file);

    printf("Conversión de %s a %s completada.\n", bmp_path, jpeg_path);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <ruta_bmp> <ruta_jpeg>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *bmp_path = argv[1];
    const char *jpeg_path = argv[2];

    convert_bmp_to_jpeg(bmp_path, jpeg_path);

    return 0;
}