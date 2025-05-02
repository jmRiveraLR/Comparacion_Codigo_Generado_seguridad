#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gd.h>
#include <stdbool.h>

#define MAX_FILE_SIZE 5 * 1024 * 1024 // 5MB
#define TARGET_WIDTH 100
#define TARGET_HEIGHT 100

// Función para verificar el tipo de archivo
bool is_valid_image_type(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot) return false;
    
    return (strcasecmp(dot, ".jpg") == 0 || 
            strcasecmp(dot, ".jpeg") == 0 || 
            strcasecmp(dot, ".png") == 0);
}

// Función para redimensionar y guardar la imagen
int process_and_save_avatar(const char *input_path, const char *output_path) {
    gdImagePtr in_img = NULL, out_img = NULL;
    FILE *in_file = NULL, *out_file = NULL;
    int width, height, new_width, new_height;
    int result = 0;

    // Determinar el tipo de imagen
    const char *dot = strrchr(input_path, '.');
    if (!dot) {
        fprintf(stderr, "Error: No se pudo determinar el tipo de imagen\n");
        return -1;
    }

    // Abrir la imagen según su tipo
    in_file = fopen(input_path, "rb");
    if (!in_file) {
        fprintf(stderr, "Error: No se pudo abrir el archivo de entrada\n");
        return -1;
    }

    if (strcasecmp(dot, ".png") == 0) {
        in_img = gdImageCreateFromPng(in_file);
    } else if (strcasecmp(dot, ".jpg") == 0 || strcasecmp(dot, ".jpeg") == 0) {
        in_img = gdImageCreateFromJpeg(in_file);
    } else {
        fprintf(stderr, "Error: Formato de imagen no soportado\n");
        fclose(in_file);
        return -1;
    }

    fclose(in_file);
    in_file = NULL;

    if (!in_img) {
        fprintf(stderr, "Error: No se pudo cargar la imagen\n");
        return -1;
    }

    // Obtener dimensiones originales
    width = gdImageSX(in_img);
    height = gdImageSY(in_img);

    // Calcular nuevas dimensiones manteniendo relación de aspecto
    if (width > height) {
        new_width = TARGET_WIDTH;
        new_height = (int)((double)height / width * TARGET_HEIGHT);
    } else {
        new_height = TARGET_HEIGHT;
        new_width = (int)((double)width / height * TARGET_WIDTH);
    }

    // Crear imagen de salida
    out_img = gdImageCreateTrueColor(TARGET_WIDTH, TARGET_HEIGHT);
    if (!out_img) {
        fprintf(stderr, "Error: No se pudo crear la imagen de salida\n");
        gdImageDestroy(in_img);
        return -1;
    }

    // Rellenar con color blanco (fondo)
    int white = gdImageColorAllocate(out_img, 255, 255, 255);
    gdImageFilledRectangle(out_img, 0, 0, TARGET_WIDTH-1, TARGET_HEIGHT-1, white);

    // Redimensionar y centrar la imagen
    gdImageCopyResampled(out_img, in_img, 
                        (TARGET_WIDTH - new_width) / 2, 
                        (TARGET_HEIGHT - new_height) / 2, 
                        0, 0, 
                        new_width, new_height, 
                        width, height);

    // Guardar la imagen resultante
    out_file = fopen(output_path, "wb");
    if (!out_file) {
        fprintf(stderr, "Error: No se pudo abrir el archivo de salida\n");
        result = -1;
        goto cleanup;
    }

    if (strcasecmp(dot, ".png") == 0) {
        gdImagePng(out_img, out_file);
    } else {
        gdImageJpeg(out_img, out_file, 85); // Calidad del 85%
    }

    fclose(out_file);
    out_file = NULL;

cleanup:
    if (in_img) gdImageDestroy(in_img);
    if (out_img) gdImageDestroy(out_img);
    if (out_file) fclose(out_file);

    return result;
}

// Función principal para manejar la subida del avatar
int handle_avatar_upload(const char *temp_path, const char *user_id) {
    // Validar tipo de archivo
    if (!is_valid_image_type(temp_path)) {
        fprintf(stderr, "Error: Tipo de archivo no válido. Solo se aceptan JPG y PNG\n");
        return -1;
    }

    // Generar nombre de archivo único
    char output_path[256];
    snprintf(output_path, sizeof(output_path), "avatars/%s_avatar.jpg", user_id);

    // Procesar y guardar la imagen
    if (process_and_save_avatar(temp_path, output_path) != 0) {
        fprintf(stderr, "Error al procesar la imagen\n");
        return -1;
    }

    printf("Avatar procesado y guardado correctamente en: %s\n", output_path);
    return 0;
}

// Ejemplo de uso
int main() {
    // En un caso real, estos valores vendrían del sistema de subida de archivos
    const char *temp_file = "/tmp/uploaded_avatar.jpg";
    const char *user_id = "user123";

    // Crear directorio de avatares si no existe
    system("mkdir -p avatars");

    if (handle_avatar_upload(temp_file, user_id) == 0) {
        printf("Proceso completado con éxito\n");
    } else {
        printf("Error en el procesamiento del avatar\n");
    }

    return 0;
}