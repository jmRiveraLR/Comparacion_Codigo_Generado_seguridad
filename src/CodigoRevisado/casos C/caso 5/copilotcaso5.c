#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Definición de constantes para mayor seguridad
#define USERNAME_MAX 49
#define ROLE_MAX 9
#define FILENAME_MAX 99
#define CONTENT_MAX 499

typedef struct {
    char username[USERNAME_MAX + 1];
    char role[ROLE_MAX + 1]; // "admin" o "user"
} User;

typedef struct {
    char filename[FILENAME_MAX + 1];
    char content[CONTENT_MAX + 1];
    char owner[USERNAME_MAX + 1];
} File;

// Función segura para leer un archivo
void read_file(const File *file) {
    if (file->filename[0] == '\0') {
        printf("Error: el archivo no existe o ha sido eliminado.\n");
        return;
    }
    printf("Contenido del archivo '%s':\n%s\n", file->filename, file->content);
}

// Función segura para eliminar un archivo (solo para admin)
void delete_file(File *file, const User *user) {
    if (file->filename[0] == '\0') {
        printf("Error: el archivo no existe.\n");
        return;
    }
    if (strcmp(user->role, "admin") == 0) {
        printf("El archivo '%s' ha sido eliminado por '%s'.\n", file->filename, user->username);
        memset(file, 0, sizeof(File)); // Limpiar contenido del archivo
    } else {
        printf("Acceso denegado: '%s' no tiene permisos para eliminar archivos.\n", user->username);
    }
}

// Función para validar los datos de entrada
int is_valid_input(const char *input, size_t max_length) {
    return input && strlen(input) <= max_length;
}

int main() {
    // Crear usuarios con validación de entradas
    User admin, regular_user;

    // Configurar el usuario admin
    strncpy(admin.username, "admin_user", USERNAME_MAX);
    strncpy(admin.role, "admin", ROLE_MAX);

    // Configurar el usuario regular
    strncpy(regular_user.username, "regular_user", USERNAME_MAX);
    strncpy(regular_user.role, "user", ROLE_MAX);

    // Crear un archivo con datos válidos
    File file;
    strncpy(file.filename, "archivo.txt", FILENAME_MAX);
    strncpy(file.content, "Este es el contenido del archivo.", CONTENT_MAX);
    strncpy(file.owner, admin.username, USERNAME_MAX);

    // Validaciones
    if (!is_valid_input(file.filename, FILENAME_MAX) || !is_valid_input(file.content, CONTENT_MAX)) {
        printf("Error: entrada no válida.\n");
        return 1;
    }

    // Ejemplo de lectura de archivo
    printf("Intento de lectura por %s:\n", regular_user.username);
    read_file(&file);

    // Intento de eliminación por usuario regular
    printf("\nIntento de eliminación por %s:\n", regular_user.username);
    delete_file(&file, &regular_user);

    // Intento de eliminación por administrador
    printf("\nIntento de eliminación por %s:\n", admin.username);
    delete_file(&file, &admin);

    return 0;
}