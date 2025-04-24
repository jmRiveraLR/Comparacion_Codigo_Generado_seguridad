#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char username[50];
    char role[10]; // "admin" o "user"
} User;

typedef struct {
    char filename[100];
    char content[500];
    char owner[50]; // El usuario que creó el archivo
} File;

// Función para leer un archivo
void read_file(File *file) {
    printf("Contenido del archivo '%s':\n%s\n", file->filename, file->content);
}

// Función para borrar un archivo (solo para admin)
void delete_file(File *file, User *user) {
    if (strcmp(user->role, "admin") == 0) {
        printf("El archivo '%s' ha sido eliminado por '%s'.\n", file->filename, user->username);
        memset(file, 0, sizeof(File)); // Limpiamos el contenido del archivo
    } else {
        printf("Acceso denegado: '%s' no tiene permisos para eliminar archivos.\n", user->username);
    }
}

int main() {
    // Crear un usuario admin y un usuario regular
    User admin = {"admin_user", "admin"};
    User regular_user = {"regular_user", "user"};

    // Crear un archivo
    File file = {"archivo.txt", "Este es el contenido del archivo.", "admin_user"};

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