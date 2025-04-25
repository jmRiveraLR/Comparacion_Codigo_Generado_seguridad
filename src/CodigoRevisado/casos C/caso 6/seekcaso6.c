#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jpeglib.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

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

// Función para apertura segura de archivos
FILE* open_secure(const char* path, const char* mode, int is_output) {
    struct stat st;
    
    // Verificar que la ruta es absoluta y no contiene componentes peligrosos
    if (strstr(path, "../") != NULL || strstr(path, "/..") != NULL) {
        fprintf(stderr, "Error: Ruta no permitida\n");
        return NULL;
    }

    // Para archivos de salida, verificar que el directorio existe y es seguro
    if (is_output) {
        char* dir = strdup(path);
        char* last_slash = strrchr(dir, '/');
        if (last_slash) {
            *last_slash = '\0';
            if (stat(dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
                free(dir);
                return NULL;
            }
            free(dir);
        }
    }

    int fd;
    if (is_output) {
        fd = open(path, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    } else {
        fd = open(path, O_RDONLY | O_NOFOLLOW);
    }
    
    if (fd == -1) {
        perror("Error al abrir archivo");
        return NULL;
    }

    // Verificar que es un archivo regular
    if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode)) {
        close(fd);
        return NULL;
    }

    FILE* file = fdopen(fd, is_output ? "wb" : "rb");
    if (!file) {
        close(fd);
    }
    return file;
}

// Función para verificar encabezado BMP
int validate_bmp_headers(BMPHeader* header, BMPInfoHeader* infoHeader) {
    // Verificar firma BMP
    if (header->type != 0x4D42) {
        fprintf(stderr, "Error: No es un archivo BMP válido\n");
        return 0;
    }

    // Verificar bits por pixel
    if (infoHeader->bitsPerPixel != 24) {
        fprintf(stderr, "Error: Solo se soportan BMP de 24 bits\n");
        return 0;
    }

    // Verificar compresión
    if (infoHeader->compression != 0) {
        fprintf(stderr, "Error: No se soportan BMP comprimidos\n");
        return 0;
    }

    // Verificar dimensiones razonables
    if (infoHeader->width <= 0 || infoHeader->width > 16384 || 
        infoHeader->height <= 0 || infoHeader->height > 16384) {
        fprintf(stderr, "Error: Dimensiones de imagen no válidas\n");
        return 0;
    }

    // Verificar posibles overflows en cálculos de tamaño
    if (infoHeader->width > INT_MAX / 3 / infoHeader->height) {
        fprintf(stderr, "Error: Imagen demasiado grande\n");
        return 0;
    }

    return 1;
}

void convertBMPtoJPEG(const char* bmpPath, const char* jpegPath, int quality) {
    // Validar parámetros de calidad
    if (quality < 1 || quality > 100) {
        fprintf(stderr, "Error: Calidad JPEG debe estar entre 1 y 100\n");
        return;
    }

    // Apertura segura de archivos
    FILE* bmpFile = open_secure(bmpPath, "rb", 0);
    if (!bmpFile) {
        fprintf(stderr, "Error: No se pudo abrir el archivo BMP %s\n", bmpPath);
        return;
    }

    BMPHeader header;
    BMPInfoHeader infoHeader;

    // Lectura segura de encabezados
    if (fread(&header, sizeof(BMPHeader), 1, bmpFile) != 1 ||
        fread(&infoHeader, sizeof(BMPInfoHeader), 1, bmpFile) != 1) {
        fprintf(stderr, "Error: No se pudo leer encabezados BMP\n");
        fclose(bmpFile);
        return;
    }

    // Validar encabezados BMP
    if (!validate_bmp_headers(&header, &infoHeader)) {
        fclose(bmpFile);
        return;
    }

    // Configuración JPEG
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    FILE* jpegFile = open_secure(jpegPath, "wb", 1);
    if (!jpegFile) {
        fprintf(stderr, "Error: No se pudo crear el archivo JPEG %s\n", jpegPath);
        fclose(bmpFile);
        jpeg_destroy_compress(&cinfo);
        return;
    }

    jpeg_stdio_dest(&cinfo, jpegFile);

    cinfo.image_width = infoHeader.width;
    cinfo.image_height = infoHeader.height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);
    jpeg_start_compress(&cinfo, TRUE);

    // Calcular padding de filas con verificación de overflow
    int row_stride = infoHeader.width * 3;
    int rowPadding = (4 - (row_stride % 4)) % 4;

    // Asignación segura de memoria para filas
    unsigned char* row = (unsigned char*)malloc(row_stride);
    if (!row) {
        fprintf(stderr, "Error: No se pudo asignar memoria\n");
        fclose(bmpFile);
        fclose(jpegFile);
        jpeg_destroy_compress(&cinfo);
        return;
    }

    JSAMPROW rowPointer[1];
    rowPointer[0] = row;

    // Posicionamiento seguro en archivo
    if (fseek(bmpFile, header.offset, SEEK_SET) != 0) {
        fprintf(stderr, "Error: Archivo BMP corrupto\n");
        free(row);
        fclose(bmpFile);
        fclose(jpegFile);
        jpeg_destroy_compress(&cinfo);
        return;
    }

    // Procesamiento de filas
    for (int y = infoHeader.height - 1; y >= 0; y--) {
        if (fread(row, 3, infoHeader.width, bmpFile) != infoHeader.width) {
            fprintf(stderr, "Error: Lectura de datos BMP fallida\n");
            break;
        }
        
        if (rowPadding > 0 && fseek(bmpFile, rowPadding, SEEK_CUR) != 0) {
            fprintf(stderr, "Error: Archivo BMP corrupto (padding)\n");
            break;
        }
        
        // Convertir BGR a RGB
        for (int x = 0; x < infoHeader.width; x++) {
            unsigned char tmp = row[x * 3];
            row[x * 3] = row[x * 3 + 2];
            row[x * 3 + 2] = tmp;
        }
        
        jpeg_write_scanlines(&cinfo, rowPointer, 1);
    }

    // Liberación segura de recursos
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

    // Validación de extensiones
    const char* bmpExt = strrchr(bmpPath, '.');
    const char* jpegExt = strrchr(jpegPath, '.');
    
    if (!bmpExt || strcasecmp(bmpExt, ".bmp") != 0) {
        fprintf(stderr, "Error: El archivo de entrada debe ser .bmp\n");
        return 1;
    }
    
    if (!jpegExt || (strcasecmp(jpegExt, ".jpg") != 0 && strcasecmp(jpegExt, ".jpeg") != 0)) {
        fprintf(stderr, "Error: El archivo de salida debe ser .jpg o .jpeg\n");
        return 1;
    }

    convertBMPtoJPEG(bmpPath, jpegPath, 90);
    return 0;
}