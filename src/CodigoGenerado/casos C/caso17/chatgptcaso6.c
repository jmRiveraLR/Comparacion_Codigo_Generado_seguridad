#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>
#include <string.h>

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

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s entrada.bmp salida.jpg\n", argv[0]);
        return 1;
    }

    FILE *bmp = fopen(argv[1], "rb");
    if (!bmp) {
        perror("Error abriendo archivo BMP");
        return 1;
    }

    BITMAPFILEHEADER file_header;
    BITMAPINFOHEADER info_header;

    fread(&file_header, sizeof(file_header), 1, bmp);
    fread(&info_header, sizeof(info_header), 1, bmp);

    if (file_header.bfType != 0x4D42 || info_header.biBitCount != 24) {
        fprintf(stderr, "Formato BMP no soportado (solo 24 bits).\n");
        fclose(bmp);
        return 1;
    }

    int width = info_header.biWidth;
    int height = abs(info_header.biHeight);
    int padding = (4 - (width * 3) % 4) % 4;

    unsigned char *data = (unsigned char *)malloc(width * height * 3);
    if (!data) {
        perror("Error reservando memoria");
        fclose(bmp);
        return 1;
    }

    fseek(bmp, file_header.bfOffBits, SEEK_SET);
    for (int y = height - 1; y >= 0; y--) {
        for (int x = 0; x < width; x++) {
            unsigned char bgr[3];
            fread(bgr, 3, 1, bmp);
            int index = (y * width + x) * 3;
            data[index] = bgr[2];     // R
            data[index + 1] = bgr[1]; // G
            data[index + 2] = bgr[0]; // B
        }
        fseek(bmp, padding, SEEK_CUR);
    }
    fclose(bmp);

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
    fclose(jpeg);
    free(data);

    printf("Conversión completada: %s -> %s\n", argv[1], argv[2]);
    return 0;
}
