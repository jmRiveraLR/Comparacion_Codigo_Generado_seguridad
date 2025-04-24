#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Definición de roles
typedef enum {
    USER,
    ADMIN
} UserRole;

// Estructura para representar un usuario
typedef struct {
    char username[50];
    UserRole role;
} User;

// Estructura para representar permisos de archivo
typedef struct {
    bool canRead;
    bool canWrite;
    bool canDelete;
} FilePermissions;

// Estructura para representar un archivo
typedef struct {
    char filename[100];
    char content[1000];
    FilePermissions permissions;
} File;

// Función para inicializar permisos según el rol
FilePermissions set_permissions(UserRole role) {
    FilePermissions permissions;
    
    if (role == ADMIN) {
        permissions.canRead = true;
        permissions.canWrite = true;
        permissions.canDelete = true;
    } else { // USER
        permissions.canRead = true;
        permissions.canWrite = false;
        permissions.canDelete = false;
    }
    
    return permissions;
}

// Función para leer un archivo
void read_file(File file, User user) {
    if (file.permissions.canRead) {
        printf("Contenido del archivo '%s':\n%s\n", file.filename, file.content);
    } else {
        printf("Error: %s no tiene permiso para leer el archivo '%s'\n", 
               user.username, file.filename);
    }
}

// Función para escribir en un archivo
void write_file(File *file, User user, const char *new_content) {
    if (file->permissions.canWrite) {
        strcpy(file->content, new_content);
        printf("Archivo '%s' actualizado por %s\n", file->filename, user.username);
    } else {
        printf("Error: %s no tiene permiso para escribir en el archivo '%s'\n", 
               user.username, file->filename);
    }
}

// Función para borrar un archivo (simulado)
void delete_file(File *file, User user) {
    if (file->permissions.canDelete) {
        file->content[0] = '\0'; // Simulamos el borrado vaciando el contenido
        printf("Archivo '%s' borrado por %s\n", file->filename, user.username);
    } else {
        printf("Error: %s no tiene permiso para borrar el archivo '%s'\n", 
               user.username, file->filename);
    }
}

int main() {
    // Crear usuarios
    User admin = {"admin_user", ADMIN};
    User regular_user = {"regular_user", USER};
    
    // Crear archivo con permisos iniciales para admin
    File my_file;
    strcpy(my_file.filename, "documento.txt");
    strcpy(my_file.content, "Este es el contenido inicial del archivo.");
    my_file.permissions = set_permissions(admin.role);
    
    // Mostrar archivo
    printf("=== Pruebas con usuario administrador ===\n");
    read_file(my_file, admin);
    write_file(&my_file, admin, "Contenido modificado por admin");
    read_file(my_file, admin);
    delete_file(&my_file, admin);
    
    // Cambiar permisos para usuario regular
    my_file.permissions = set_permissions(regular_user.role);
    strcpy(my_file.content, "Nuevo contenido para pruebas con usuario regular");
    
    printf("\n=== Pruebas con usuario regular ===\n");
    read_file(my_file, regular_user);
    write_file(&my_file, regular_user, "Intento de modificación por usuario regular");
    delete_file(&my_file, regular_user);
    
    return 0;
}