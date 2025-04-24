#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>

void convert_bmp_to_jpeg(const char *bmp_path, const char *jpeg_path) {
    FILE *bmp_file = fopen(bmp_path, "rb");
    if (!bmp_file) {
        perror("No se pudo abrir el archivo BMP");
        exit(EXIT_FAILURE);
    }

    FILE *jpeg_file = fopen(jpeg_path, "wb");
    if (!jpeg_file) {
        perror("No se pudo abrir el archivo JPEG");
        fclose(bmp_file);
        exit(EXIT_FAILURE);
    }

    // Leer encabezado BMP (simplificado para este ejemplo)
    unsigned char bmp_header[54];
    fread(bmp_header, sizeof(unsigned char), 54, bmp_file);

    int width = *(int *)&bmp_header[18];
    int height = *(int *)&bmp_header[22];

    unsigned char *bmp_data = malloc(3 * width * height);
    if (!bmp_data) {
        perror("Error al asignar memoria para los datos BMP");
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