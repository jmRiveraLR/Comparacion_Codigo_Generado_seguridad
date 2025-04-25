#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>

#define BUFFER_SIZE 4096
#define BACKUP_DIR "respaldo/"
#define MAX_PATH_LEN 4096 // Tamaño máximo razonable para rutas en sistemas modernos

int is_valid_path(const char *path) {
    // Validación básica de ruta absoluta
    if (path == NULL || path[0] != '/') {
        return 0;
    }

    // Verificar que no contenga componentes inseguros
    if (strstr(path, "/../") || strstr(path, "//") || strstr(path, "/./")) {
        return 0;
    }

    // Verificar longitud máxima
    if (strlen(path) > MAX_PATH_LEN) {
        return 0;
    }

    return 1;
}

int safe_copy_file(const char *src_path, const char *dest_path) {
    int src_fd = -1, dest_fd = -1;
    ssize_t bytes_read, bytes_written;
    char buffer[BUFFER_SIZE];
    struct stat src_stat;

    // Abrir archivo origen con validaciones de seguridad
    if ((src_fd = open(src_path, O_RDONLY | O_NOFOLLOW)) == -1) {
        perror("Error al abrir archivo origen");
        return -1;
    }

    // Obtener información del archivo origen
    if (fstat(src_fd, &src_stat) == -1) {
        perror("Error al obtener información del archivo");
        close(src_fd);
        return -1;
    }

    // Verificar que es un archivo regular
    if (!S_ISREG(src_stat.st_mode)) {
        fprintf(stderr, "Error: El origen no es un archivo regular\n");
        close(src_fd);
        return -1;
    }

    // Crear archivo destino con permisos seguros
    mode_t old_umask = umask(0077); // Permisos restrictivos
    if ((dest_fd = open(dest_path, O_WRONLY | O_CREAT | O_EXCL, 0600)) == -1) {
        perror("Error al crear archivo destino");
        umask(old_umask);
        close(src_fd);
        return -1;
    }
    umask(old_umask);

    // Copiar contenido
    while ((bytes_read = read(src_fd, buffer, BUFFER_SIZE)) > 0) {
        if (bytes_read == -1) {
            perror("Error al leer archivo origen");
            close(src_fd);
            close(dest_fd);
            unlink(dest_path); // Eliminar archivo parcialmente copiado
            return -1;
        }

        bytes_written = write(dest_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            perror("Error al escribir archivo destino");
            close(src_fd);
            close(dest_fd);
            unlink(dest_path); // Eliminar archivo parcialmente copiado
            return -1;
        }
    }

    // Cerrar archivos
    close(src_fd);
    if (close(dest_fd) == -1) {
        perror("Error al cerrar archivo destino");
        unlink(dest_path);
        return -1;
    }

    return 0;
}

int main() {
    char filepath[MAX_PATH_LEN + 1] = {0};
    char backup_path[MAX_PATH_LEN + 1] = {0};
    char *filename;
    struct stat dir_stat;

    // Leer la ruta del archivo con límite seguro
    printf("Ingrese la ruta absoluta del archivo a respaldar: ");
    if (fgets(filepath, sizeof(filepath), stdin) == NULL) {
        perror("Error al leer la entrada");
        return 1;
    }

    // Eliminar salto de línea y validar entrada
    filepath[strcspn(filepath, "\n")] = '\0';
    if (!is_valid_path(filepath)) {
        fprintf(stderr, "Error: Ruta inválida o insegura\n");
        return 1;
    }

    // Verificar existencia del directorio respaldo de forma segura
    if (stat(BACKUP_DIR, &dir_stat) == -1) {
        if (errno == ENOENT) {
            // Crear directorio con permisos seguros
            mode_t old_umask = umask(0077);
            if (mkdir(BACKUP_DIR, 0700) == -1) {
                perror("Error al crear directorio respaldo");
                umask(old_umask);
                return 1;
            }
            umask(old_umask);
        } else {
            perror("Error al verificar directorio respaldo");
            return 1;
        }
    } else if (!S_ISDIR(dir_stat.st_mode)) {
        fprintf(stderr, "Error: %s existe pero no es un directorio\n", BACKUP_DIR);
        return 1;
    }

    // Extraer nombre del archivo de forma segura
    if ((filename = strrchr(filepath, '/')) == NULL || *(filename + 1) == '\0') {
        fprintf(stderr, "Error: No se pudo determinar el nombre del archivo\n");
        return 1;
    }
    filename++; // Saltar el '/'

    // Construir ruta de destino con verificación de longitud
    if (snprintf(backup_path, sizeof(backup_path), "%s%s", BACKUP_DIR, filename) >= sizeof(backup_path)) {
        fprintf(stderr, "Error: Ruta de destino demasiado larga\n");
        return 1;
    }

    // Realizar copia segura
    if (safe_copy_file(filepath, backup_path) != 0) {
        fprintf(stderr, "Error: No se pudo completar el respaldo\n");
        return 1;
    }

    printf("Archivo copiado exitosamente a %s\n", backup_path);
    return 0;
}