#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILENAME 100
#define MAX_USERS 10
#define MAX_CONTENT 1024
#define MAX_USERNAME 50

typedef enum { USER, ADMIN } Role;

typedef struct {
    char username[MAX_USERNAME];
    Role role;
} User;

typedef struct {
    char filename[MAX_FILENAME];
    char content[MAX_CONTENT];
    User owner;
} File;

File fileSystem;

void safe_strncpy(char *dest, const char *src, size_t size) {
    if (size == 0) return;
    strncpy(dest, src, size - 1);
    dest[size - 1] = '\0';
}

int is_valid_filename(const char *filename) {
    return filename != NULL && strlen(filename) < MAX_FILENAME &&
           strstr(filename, "..") == NULL && strchr(filename, '/') == NULL;
}

void createFile(const char *filename, const char *content, User user) {
    if (!is_valid_filename(filename)) {
        printf("Nombre de archivo no válido.\n");
        return;
    }

    safe_strncpy(fileSystem.filename, filename, MAX_FILENAME);
    safe_strncpy(fileSystem.content, content, MAX_CONTENT);
    fileSystem.owner = user;
    printf("Archivo '%s' creado por '%s'.\n", fileSystem.filename, user.username);
}

void readFile(User user) {
    printf("[%s] Leyendo archivo '%s':\n%s\n", user.username, fileSystem.filename, fileSystem.content);
}

void deleteFile(User user) {
    if (user.role != ADMIN) {
        printf("Error: '%s' no tiene permisos para borrar archivos.\n", user.username);
        return;
    }
    memset(&fileSystem, 0, sizeof(File));
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
