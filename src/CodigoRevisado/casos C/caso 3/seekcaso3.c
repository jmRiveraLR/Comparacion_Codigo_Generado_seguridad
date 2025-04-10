#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024 * (EVENT_SIZE + NAME_MAX + 1))
#define MAX_LOG_SIZE (10 * 1024 * 1024) // 10MB máximo para el log

void log_event(const char *log_file, const char *event_type, const char *filename) {
    // Verificar longitud de filename
    if (filename == NULL || strlen(filename) > NAME_MAX) {
        fprintf(stderr, "Nombre de archivo inválido\n");
        return;
    }

    // Abrir archivo con protecciones contra TOCTOU (CWE-362)
    int fd = open(log_file, O_WRONLY|O_CREAT|O_APPEND|O_NOFOLLOW|O_CLOEXEC, 0600);
    if (fd == -1) {
        perror("Error al abrir archivo de log");
        return;
    }

    // Verificar tamaño del log (CWE-400)
    struct stat st;
    if (fstat(fd, &st) == 0 && st.st_size > MAX_LOG_SIZE) {
        fprintf(stderr, "Archivo de log excede tamaño máximo\n");
        close(fd);
        return;
    }

    FILE *log = fdopen(fd, "a");
    if (log == NULL) {
        perror("Error al abrir stream de log");
        close(fd);
        return;
    }

    // Obtener timestamp de forma segura (CWE-126)
    time_t now;
    time(&now);
    char timestamp[26];
    ctime_r(&now, timestamp);
    timestamp[24] = '\0'; // Eliminar newline

    // Escribir log (CWE-20)
    if (fprintf(log, "[%s] %s: %s\n", timestamp, event_type, filename) < 0) {
        perror("Error al escribir en log");
    }

    fclose(log);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <directorio> <archivo_log>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Validar paths (CWE-22)
    char real_dir[PATH_MAX];
    char real_log[PATH_MAX];
    if (realpath(argv[1], real_dir) == NULL || realpath(argv[2], real_log) == NULL) {
        perror("Error al validar paths");
        return EXIT_FAILURE;
    }

    int fd = inotify_init1(IN_CLOEXEC);
    if (fd < 0) {
        perror("inotify_init");
        return EXIT_FAILURE;
    }

    // Configurar watch de forma segura (CWE-119/120)
    int wd = inotify_add_watch(fd, real_dir, 
                             IN_MODIFY | IN_CREATE | IN_DELETE | 
                             IN_MOVED_FROM | IN_MOVED_TO);
    if (wd == -1) {
        perror("inotify_add_watch");
        close(fd);
        return EXIT_FAILURE;
    }

    char buffer[BUF_LEN] __attribute__ ((aligned(8))); // Buffer alineado
    printf("Monitorizando: %s (Log: %s)\n", real_dir, real_log);

    while (1) {
        ssize_t len = read(fd, buffer, BUF_LEN - 1);
        if (len < 0) {
            if (errno == EINTR) continue;
            perror("read");
            break;
        }

        for (char *ptr = buffer; ptr < buffer + len; ) {
            struct inotify_event *event = (struct inotify_event *)ptr;
            
            if (event->len) {
                // Validar nombre del archivo (CWE-20, CWE-22)
                if (strnlen(event->name, NAME_MAX + 1) > NAME_MAX) {
                    fprintf(stderr, "Nombre de archivo demasiado largo\n");
                    continue;
                }

                const char *type = NULL;
                if (event->mask & IN_ISDIR) {
                    if (event->mask & IN_CREATE) type = "SUBDIR_CREADO";
                    else if (event->mask & IN_MODIFY) type = "SUBDIR_MODIFICADO";
                    else if (event->mask & IN_DELETE) type = "SUBDIR_ELIMINADO";
                    else if (event->mask & IN_MOVED_FROM) type = "SUBDIR_MOVIDO/ELIMINADO";
                    else if (event->mask & IN_MOVED_TO) type = "SUBDIR_MOVIDO/CREADO";
                } else {
                    if (event->mask & IN_CREATE) type = "ARCHIVO_CREADO";
                    else if (event->mask & IN_MODIFY) type = "ARCHIVO_MODIFICADO";
                    else if (event->mask & IN_DELETE) type = "ARCHIVO_ELIMINADO";
                    else if (event->mask & IN_MOVED_FROM) type = "ARCHIVO_MOVIDO/ELIMINADO";
                    else if (event->mask & IN_MOVED_TO) type = "ARCHIVO_MOVIDO/CREADO";
                }

                if (type) {
                    log_event(real_log, type, event->name);
                }
            }
            ptr += sizeof(struct inotify_event) + event->len;
        }
    }

    inotify_rm_watch(fd, wd);
    close(fd);
    return EXIT_SUCCESS;
}