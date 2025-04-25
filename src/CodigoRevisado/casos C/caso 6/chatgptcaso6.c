#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jpeglib.h>
#include <errno.h>
#include <sys/stat.h>

#pragma pack(push, 1)
typedef struct {
    unsigned short bfType;
    unsigned int bfSize;
    unsigned short bfReserved1;
    unsigned short bfReserved2;
    unsigned int bfOffBits;
} BITMAPFILEHEADER;

typedef struct {
    unsigned int biSize;
    int          biWidth;
    int          biHeight;
    unsigned short biPlanes;
    unsigned short biBitCount;
    unsigned int biCompression;
    unsigned int biSizeImage;
    int          biXPelsPerMeter;
    int          biYPelsPerMeter;
    unsigned int biClrUsed;
    unsigned int biClrImportant;
} BITMAPINFOHEADER;
#pragma pack(pop)

// Validación de archivo regular
int is_regular_file(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return S_ISREG(st.st_mode);
}

int safe_fread(void *ptr, size_t size, size_t count, FILE *stream) {
    size_t r = fread(ptr, size, count, stream);
    return (r == count);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s entrada.bmp salida.jpg\n", argv[0]);
        return 1;
    }

    if (!is_regular_file(argv[1])) {
        fprintf(stderr, "El archivo de entrada no es válido.\n");
        return 1;
    }

    FILE *bmp = fopen(argv[1], "rb");
    if (!bmp) {
        perror("Error abriendo archivo BMP");
        return 1;
    }

    BITMAPFILEHEADER file_header;
    BITMAPINFOHEADER info_header;

    if (!safe_fread(&file_header, sizeof(file_header), 1, bmp) ||
        !safe_fread(&info_header, sizeof(info_header), 1, bmp)) {
        fprintf(stderr, "Error leyendo encabezados BMP\n");
        fclose(bmp);
        return 1;
    }

    if (file_header.bfType != 0x4D42 || info_header.biBitCount != 24) {
        fprintf(stderr, "Formato BMP no soportado (solo 24 bits).\n");
        fclose(bmp);
        return 1;
    }

    int width = info_header.biWidth;
    int height = abs(info_header.biHeight);
    int padding = (4 - (width * 3) % 4) % 4;

    size_t image_size = (size_t)width * height * 3;
    unsigned char *data = malloc(image_size);
    if (!data) {
        perror("Error reservando memoria");
        fclose(bmp);
        return 1;
    }

    if (fseek(bmp, file_header.bfOffBits, SEEK_SET) != 0) {
        perror("Error moviendo puntero de archivo BMP");
        free(data);
        fclose(bmp);
        return 1;
    }

    for (int y = height - 1; y >= 0; y--) {
        for (int x = 0; x < width; x++) {
            unsigned char bgr[3];
            if (!safe_fread(bgr, 3, 1, bmp)) {
                fprintf(stderr, "Error leyendo datos de píxeles BMP\n");
                free(data);
                fclose(bmp);
                return 1;
            }
            size_t index = (size_t)(y * width + x) * 3;
            if (index + 2 >= image_size) {
                fprintf(stderr, "Error: índice fuera de límites\n");
                free(data);
                fclose(bmp);
                return 1;
            }
            data[index]     = bgr[2]; // R
            data[index + 1] = bgr[1]; // G
            data[index + 2] = bgr[0]; // B
        }
        if (fseek(bmp, padding, SEEK_CUR) != 0) {
            perror("Error leyendo padding de fila");
            free(data);
            fclose(bmp);
            return 1;
        }
    }

    if (fclose(bmp) != 0) {
        perror("Error cerrando archivo BMP");
        free(data);
        return 1;
    }

    FILE *jpeg = fopen(argv[2], "wb");
    if (!jpeg) {
        perror("Error creando archivo JPEG");
        free(data);
        return 1;
    }

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, jpeg);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_start_compress(&cinfo, TRUE);

    while (cinfo.next_scanline < cinfo.image_height) {
        JSAMPROW row_pointer = &data[cinfo.next_scanline * width * 3];
        jpeg_write_scanlines(&cinfo, &row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    if (fclose(jpeg) != 0) {
        perror("Error cerrando archivo JPEG");
        free(data);
        return 1;
    }

    free(data);
    printf("Conversión segura completada: %s -> %s\n", argv[1], argv[2]);
    return 0;
}
