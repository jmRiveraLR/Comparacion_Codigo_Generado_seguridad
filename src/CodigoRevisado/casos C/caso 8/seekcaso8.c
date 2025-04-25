#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gd.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>

#define MAX_FILE_SIZE (5 * 1024 * 1024) // 5MB
#define TARGET_WIDTH 100
#define TARGET_HEIGHT 100
#define MAX_PATH_LEN 256
#define SAFE_DIR "avatars"
#define PERM_DIR 0755

// Función para verificar el tipo de archivo por contenido (no solo extensión)
bool is_valid_image_content(FILE *file) {
    unsigned char header[8];
    if (fread(header, sizeof(header), 1, file) != 1) {
        return false;
    }
    rewind(file);

    // Verificar magic numbers para JPEG y PNG
    return (header[0] == 0xFF && header[1] == 0xD8) || // JPEG
           (header[0] == 0x89 && header[1] == 0x50 &&  // PNG
            header[2] == 0x4E && header[3] == 0x47 &&
            header[4] == 0x0D && header[5] == 0x0A &&
            header[6] == 0x1A && header[7] == 0x0A);
}

// Función para validar nombres de archivo seguros
bool is_safe_filename(const char *filename) {
    if (!filename || strlen(filename) > 64) return false;
    
    const char *forbidden_chars = "../\\~|&;`$<>(){}[]'\"";
    return strpbrk(filename, forbidden_chars) == NULL;
}

// Función segura para crear directorio
int create_avatar_dir() {
    if (mkdir(SAFE_DIR, PERM_DIR) == -1) {
        if (errno != EEXIST) {
            perror("Error al crear directorio");
            return -1;
        }
    }
    return 0;
}

// Función para redimensionar y guardar la imagen de forma segura
int process_and_save_avatar(const char *input_path, const char *user_id) {
    if (!input_path || !user_id || !is_safe_filename(user_id)) {
        fprintf(stderr, "Parámetros inválidos\n");
        return -1;
    }

    // Validar existencia y tipo del archivo de entrada
    struct stat st;
    if (lstat(input_path, &st) != 0 || !S_ISREG(st.st_mode) || 
        st.st_size > MAX_FILE_SIZE) {
        fprintf(stderr, "Archivo inválido\n");
        return -1;
    }

    // Abrir archivo con validación de contenido
    FILE *in_file = fopen(input_path, "rb");
    if (!in_file || !is_valid_image_content(in_file)) {
        fprintf(stderr, "Tipo de imagen no válido\n");
        if (in_file) fclose(in_file);
        return -1;
    }

    // Procesar imagen con GD
    gdImagePtr in_img = NULL, out_img = NULL;
    const char *dot = strrchr(input_path, '.');
    
    if (dot && (strcasecmp(dot, ".png") == 0)) {
        in_img = gdImageCreateFromPng(in_file);
    } else {
        in_img = gdImageCreateFromJpeg(in_file);
    }
    fclose(in_file);

    if (!in_img) {
        fprintf(stderr, "Error al procesar imagen\n");
        return -1;
    }

    // Crear imagen de salida
    out_img = gdImageCreateTrueColor(TARGET_WIDTH, TARGET_HEIGHT);
    if (!out_img) {
        gdImageDestroy(in_img);
        return -1;
    }

    // Configuración segura de imagen de salida
    int white = gdImageColorAllocate(out_img, 255, 255, 255);
    gdImageFilledRectangle(out_img, 0, 0, TARGET_WIDTH-1, TARGET_HEIGHT-1, white);

    // Redimensionamiento seguro
    int width = gdImageSX(in_img);
    int height = gdImageSY(in_img);
    int new_width, new_height;

    if (width > height) {
        new_width = TARGET_WIDTH;
        new_height = (int)((double)height / width * TARGET_HEIGHT);
    } else {
        new_height = TARGET_HEIGHT;
        new_width = (int)((double)width / height * TARGET_WIDTH);
    }

    gdImageCopyResampled(out_img, in_img, 
                        (TARGET_WIDTH - new_width) / 2, 
                        (TARGET_HEIGHT - new_height) / 2, 
                        0, 0, 
                        new_width, new_height, 
                        width, height);

    // Guardar de forma segura
    char output_path[MAX_PATH_LEN];
    snprintf(output_path, MAX_PATH_LEN, "%s/%s_avatar.jpg", SAFE_DIR, user_id);

    // Validar ruta segura
    if (strstr(output_path, "..") || strstr(output_path, "//")) {
        fprintf(stderr, "Ruta insegura detectada\n");
        gdImageDestroy(in_img);
        gdImageDestroy(out_img);
        return -1;
    }

    // Crear archivo de forma segura
    int fd = open(output_path, O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (fd == -1) {
        perror("Error al crear archivo");
        gdImageDestroy(in_img);
        gdImageDestroy(out_img);
        return -1;
    }

    FILE *out_file = fdopen(fd, "wb");
    if (!out_file) {
        close(fd);
        gdImageDestroy(in_img);
        gdImageDestroy(out_img);
        return -1;
    }

    gdImageJpeg(out_img, out_file, 85);
    fclose(out_file);
    gdImageDestroy(in_img);
    gdImageDestroy(out_img);

    return 0;
}

int main() {
    if (create_avatar_dir() != 0) {
        return EXIT_FAILURE;
    }

    // Ejemplo de uso - en un caso real estos valores vendrían validados
    const char *temp_file = "test.jpg";
    const char *user_id = "user123";

    if (process_and_save_avatar(temp_file, user_id) == 0) {
        printf("Avatar procesado con éxito\n");
        return EXIT_SUCCESS;
    } else {
        fprintf(stderr, "Error al procesar avatar\n");
        return EXIT_FAILURE;
    }
}