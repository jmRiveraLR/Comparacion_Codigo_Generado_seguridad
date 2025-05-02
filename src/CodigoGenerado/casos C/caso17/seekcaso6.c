#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jpeglib.h>

#pragma pack(push, 1)
typedef struct {
    unsigned short type;
    unsigned int size;
    unsigned short reserved1;
    unsigned short reserved2;
    unsigned int offset;
} BMPHeader;

typedef struct {
    unsigned int size;
    int width;
    int height;
    unsigned short planes;
    unsigned short bitsPerPixel;
    unsigned int compression;
    unsigned int imageSize;
    int xPixelsPerMeter;
    int yPixelsPerMeter;
    unsigned int colorsUsed;
    unsigned int colorsImportant;
} BMPInfoHeader;
#pragma pack(pop)

void convertBMPtoJPEG(const char* bmpPath, const char* jpegPath, int quality) {
    FILE* bmpFile = fopen(bmpPath, "rb");
    if (!bmpFile) {
        fprintf(stderr, "Error: No se pudo abrir el archivo BMP %s\n", bmpPath);
        return;
    }

    BMPHeader header;
    BMPInfoHeader infoHeader;

    // Leer encabezados BMP
    fread(&header, sizeof(BMPHeader), 1, bmpFile);
    fread(&infoHeader, sizeof(BMPInfoHeader), 1, bmpFile);

    // Verificar que es un archivo BMP válido
    if (header.type != 0x4D42) {
        fprintf(stderr, "Error: El archivo %s no es un BMP válido\n", bmpPath);
        fclose(bmpFile);
        return;
    }

    // Solo soportamos BMP de 24 bits sin compresión
    if (infoHeader.bitsPerPixel != 24 || infoHeader.compression != 0) {
        fprintf(stderr, "Error: Solo se soportan BMP de 24 bits sin compresión\n");
        fclose(bmpFile);
        return;
    }

    // Configurar parámetros para libjpeg
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    FILE* jpegFile = fopen(jpegPath, "wb");
    if (!jpegFile) {
        fprintf(stderr, "Error: No se pudo crear el archivo JPEG %s\n", jpegPath);
        fclose(bmpFile);
        jpeg_destroy_compress(&cinfo);
        return;
    }

    jpeg_stdio_dest(&cinfo, jpegFile);

    cinfo.image_width = infoHeader.width;
    cinfo.image_height = infoHeader.height;
    cinfo.input_components = 3; // RGB
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);
    jpeg_start_compress(&cinfo, TRUE);

    // Calcular el padding de filas BMP (alineamiento a 4 bytes)
    int rowPadding = (4 - (infoHeader.width * 3) % 4) % 4;

    // Leer datos BMP y escribir JPEG
    unsigned char* row = (unsigned char*)malloc(infoHeader.width * 3);
    JSAMPROW rowPointer[1];
    rowPointer[0] = row;

    fseek(bmpFile, header.offset, SEEK_SET);

    // BMP almacena las filas al revés
    for (int y = infoHeader.height - 1; y >= 0; y--) {
        fread(row, 3, infoHeader.width, bmpFile);
        fseek(bmpFile, rowPadding, SEEK_CUR); // Saltar padding
        
        // Convertir BGR a RGB
        for (int x = 0; x < infoHeader.width; x++) {
            unsigned char tmp = row[x * 3];
            row[x * 3] = row[x * 3 + 2];
            row[x * 3 + 2] = tmp;
        }
        
        jpeg_write_scanlines(&cinfo, rowPointer, 1);
    }

    free(row);
    jpeg_finish_compress(&cinfo);
    fclose(jpegFile);
    fclose(bmpFile);
    jpeg_destroy_compress(&cinfo);

    printf("Conversión completada: %s -> %s\n", bmpPath, jpegPath);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Uso: %s <archivo_entrada.bmp> <archivo_salida.jpg>\n", argv[0]);
        return 1;
    }

    const char* bmpPath = argv[1];
    const char* jpegPath = argv[2];

    // Calcular extensión del archivo de salida
    const char* ext = strrchr(jpegPath, '.');
    if (!ext || (strcmp(ext, ".jpg") != 0 && strcmp(ext, ".jpeg") != 0)) {
        fprintf(stderr, "Error: El archivo de salida debe tener extensión .jpg o .jpeg\n");
        return 1;
    }

    convertBMPtoJPEG(bmpPath, jpegPath, 90); // Calidad 90%

    return 0;
}