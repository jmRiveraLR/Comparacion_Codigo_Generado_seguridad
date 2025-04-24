#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILENAME 100
#define MAX_USERS 10

typedef enum { USER, ADMIN } Role;

typedef struct {
    char username[50];
    Role role;
} User;

typedef struct {
    char filename[MAX_FILENAME];
    char content[1024];
    User owner;
} File;

// Simulador de archivo (1 archivo para ejemplo)
File fileSystem;

// Crear archivo
void createFile(const char *filename, const char *content, User user) {
    strncpy(fileSystem.filename, filename, MAX_FILENAME);
    strncpy(fileSystem.content, content, sizeof(fileSystem.content));
    fileSystem.owner = user;
    printf("Archivo '%s' creado por '%s'.\n", filename, user.username);
}

// Leer archivo
void readFile(User user) {
    printf("[%s] Leyendo archivo '%s':\n%s\n", user.username, fileSystem.filename, fileSystem.content);
}

// Borrar archivo
void deleteFile(User user) {
    if (user.role != ADMIN) {
        printf("Error: '%s' no tiene permisos para borrar archivos.\n", user.username);
        return;
    }
    fileSystem.filename[0] = '\0';
    fileSystem.content[0] = '\0';
    printf("Archivo eliminado por '%s'.\n", user.username);
}

int main() {
    User admin = {"admin_user", ADMIN};
    User regular = {"regular_user", USER};

    createFile("documento.txt", "Este es el contenido del archivo.", admin);

    readFile(regular);   // Permitido
    deleteFile(regular); // Denegado

    deleteFile(admin);   // Permitido
    return 0;
}
