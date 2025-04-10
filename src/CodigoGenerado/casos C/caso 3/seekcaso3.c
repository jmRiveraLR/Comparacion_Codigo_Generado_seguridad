#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define EVENT_SIZE  (sizeof(struct inotify_event))
#define BUF_LEN     (1024 * (EVENT_SIZE + 16))

void log_event(const char *log_file, const char *event_type, const char *filename) {
    time_t now;
    time(&now);
    char *timestamp = ctime(&now);
    timestamp[strlen(timestamp)-1] = '\0'; // Eliminar el salto de línea
    
    FILE *log = fopen(log_file, "a");
    if (log == NULL) {
        perror("Error al abrir el archivo de log");
        return;
    }
    
    fprintf(log, "[%s] %s: %s\n", timestamp, event_type, filename);
    fclose(log);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <directorio_a_monitorizar> <archivo_log>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *dir_to_watch = argv[1];
    const char *log_file = argv[2];

    int fd;
    int wd;
    char buffer[BUF_LEN];

    // Inicializar inotify
    fd = inotify_init();
    if (fd < 0) {
        perror("inotify_init");
        exit(EXIT_FAILURE);
    }

    // Añadir watch para el directorio especificado
    wd = inotify_add_watch(fd, dir_to_watch, 
                          IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO);
    if (wd == -1) {
        perror("inotify_add_watch");
        close(fd);
        exit(EXIT_FAILURE);
    }

    printf("Monitorizando el directorio: %s (Log: %s)\n", dir_to_watch, log_file);
    printf("Presiona Ctrl+C para salir...\n");

    // Bucle principal para leer eventos
    while (1) {
        int length = read(fd, buffer, BUF_LEN);
        if (length < 0) {
            perror("read");
            break;
        }

        int i = 0;
        while (i < length) {
            struct inotify_event *event = (struct inotify_event *) &buffer[i];
            if (event->len) {
                if (event->mask & IN_CREATE) {
                    if (event->mask & IN_ISDIR) {
                        log_event(log_file, "SUBDIRECTORIO CREADO", event->name);
                    } else {
                        log_event(log_file, "ARCHIVO CREADO", event->name);
                    }
                } else if (event->mask & IN_MODIFY) {
                    if (event->mask & IN_ISDIR) {
                        log_event(log_file, "SUBDIRECTORIO MODIFICADO", event->name);
                    } else {
                        log_event(log_file, "ARCHIVO MODIFICADO", event->name);
                    }
                } else if (event->mask & IN_DELETE) {
                    if (event->mask & IN_ISDIR) {
                        log_event(log_file, "SUBDIRECTORIO ELIMINADO", event->name);
                    } else {
                        log_event(log_file, "ARCHIVO ELIMINADO", event->name);
                    }
                } else if (event->mask & IN_MOVED_FROM) {
                    if (event->mask & IN_ISDIR) {
                        log_event(log_file, "SUBDIRECTORIO MOVIDO/ELIMINADO", event->name);
                    } else {
                        log_event(log_file, "ARCHIVO MOVIDO/ELIMINADO", event->name);
                    }
                } else if (event->mask & IN_MOVED_TO) {
                    if (event->mask & IN_ISDIR) {
                        log_event(log_file, "SUBDIRECTORIO MOVIDO/CREADO", event->name);
                    } else {
                        log_event(log_file, "ARCHIVO MOVIDO/CREADO", event->name);
                    }
                }
            }
            i += EVENT_SIZE + event->len;
        }
    }

    // Limpieza
    inotify_rm_watch(fd, wd);
    close(fd);

    return 0;
}