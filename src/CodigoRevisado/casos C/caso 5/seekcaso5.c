#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>

// --------------------------
// Estructuras seguras
// --------------------------

#define MAX_USERNAME_LEN 50
#define MAX_FILENAME_LEN 100
#define MAX_CONTENT_LEN 4096

typedef enum {
    USER,
    ADMIN
} UserRole;

typedef struct {
    char username[MAX_USERNAME_LEN + 1]; // +1 para el null terminator
    UserRole role;
} User;

typedef struct {
    bool canRead;
    bool canWrite;
    bool canDelete;
} FilePermissions;

typedef struct {
    char filename[MAX_FILENAME_LEN + 1];
    char content[MAX_CONTENT_LEN + 1];
    FilePermissions permissions;
} File;

// --------------------------
// Funciones seguras
// --------------------------

// Función segura para copiar strings
void safe_strcpy(char* dest, const char* src, size_t dest_size) {
    if (dest == NULL || src == NULL || dest_size == 0) {
        return;
    }
    size_t src_len = strlen(src);
    size_t copy_len = (src_len < dest_size) ? src_len : dest_size - 1;
    strncpy(dest, src, copy_len);
    dest[copy_len] = '\0';
}

// Validación de nombres de archivo
bool is_valid_filename(const char* filename) {
    if (filename == NULL || strlen(filename) == 0 || strlen(filename) > MAX_FILENAME_LEN) {
        return false;
    }
    
    // Prevenir path traversal
    if (strstr(filename, "../") || strstr(filename, "/..") || strchr(filename, '\\')) {
        return false;
    }
    
    // Caracteres permitidos (personalizable)
    const char* allowed_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.";
    for (size_t i = 0; i < strlen(filename); i++) {
        if (!strchr(allowed_chars, filename[i])) {
            return false;
        }
    }
    
    return true;
}

FilePermissions set_permissions(UserRole role) {
    FilePermissions permissions = {false, false, false};
    
    // Principio de menor privilegio
    permissions.canRead = true; // Todos pueden leer
    
    if (role == ADMIN) {
        permissions.canWrite = true;
        permissions.canDelete = true;
    }
    
    return permissions;
}

// --------------------------
// Operaciones de archivo seguras
// --------------------------

bool read_file(const File* file, const User* user) {
    if (file == NULL || user == NULL) {
        return false;
    }
    
    if (!file->permissions.canRead) {
        printf("Error: Permiso denegado para leer '%s'\n", file->filename);
        return false;
    }
    
    printf("Contenido de '%s':\n%.*s\n", 
           file->filename, 
           MAX_CONTENT_LEN, // Prevenir overflows en printf
           file->content);
    return true;
}

bool write_file(File* file, const User* user, const char* new_content) {
    if (file == NULL || user == NULL || new_content == NULL) {
        return false;
    }
    
    if (!file->permissions.canWrite) {
        printf("Error: Permiso denegado para escribir en '%s'\n", file->filename);
        return false;
    }
    
    safe_strcpy(file->content, new_content, MAX_CONTENT_LEN + 1);
    printf("Archivo '%s' actualizado\n", file->filename);
    return true;
}

bool delete_file(File* file, const User* user) {
    if (file == NULL || user == NULL) {
        return false;
    }
    
    if (!file->permissions.canDelete) {
        printf("Error: Permiso denegado para borrar '%s'\n", file->filename);
        return false;
    }
    
    // Borrado seguro
    memset(file->content, 0, MAX_CONTENT_LEN);
    printf("Archivo '%s' marcado para borrado\n", file->filename);
    return true;
}

// --------------------------
// Autenticación y autorización mejorada
// --------------------------

bool authenticate_user(User* user, const char* username, UserRole role) {
    if (user == NULL || username == NULL || strlen(username) == 0) {
        return false;
    }
    
    // Validación de entrada
    if (strlen(username) > MAX_USERNAME_LEN) {
        return false;
    }
    
    safe_strcpy(user->username, username, MAX_USERNAME_LEN + 1);
    user->role = role;
    return true;
}

// --------------------------
// Ejemplo de uso seguro
// --------------------------

int main() {
    // Configuración segura
    User admin, regular_user;
    File document;
    
    if (!authenticate_user(&admin, "admin_secure", ADMIN) ||
        !authenticate_user(&regular_user, "user_secure", USER)) {
        printf("Error en autenticación\n");
        return EXIT_FAILURE;
    }
    
    if (!is_valid_filename("documento.txt")) {
        printf("Nombre de archivo inválido\n");
        return EXIT_FAILURE;
    }
    
    // Inicialización segura
    safe_strcpy(document.filename, "documento.txt", MAX_FILENAME_LEN + 1);
    safe_strcpy(document.content, "Contenido seguro inicial", MAX_CONTENT_LEN + 1);
    document.permissions = set_permissions(admin.role);
    
    // Operaciones
    read_file(&document, &admin);
    write_file(&document, &admin, "Contenido modificado de forma segura");
    
    // Cambio de permisos para usuario regular
    document.permissions = set_permissions(regular_user.role);
    read_file(&document, &regular_user);
    
    // Intentar operaciones no permitidas
    write_file(&document, &regular_user, "Intento no autorizado");
    delete_file(&document, &regular_user);
    
    return EXIT_SUCCESS;
}