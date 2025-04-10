#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <errno.h>

#define EVENT_SIZE  (sizeof(struct inotify_event))
#define BUF_LEN     (1024 * (EVENT_SIZE + NAME_MAX + 1))
#define LOG_FILE    "monitor_log.txt"

// Devuelve el timestamp formateado
void get_timestamp(char *buffer, size_t len) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, len, "%Y-%m-%d %H:%M:%S", t);
}

// Registra un evento en el archivo de log
void log_event(const char *event_type, const char *filename) {
    FILE *log = fopen(LOG_FILE, "a");
    if (!log) {
        perror("fopen");
        return;
    }

    char timestamp[64];
    get_timestamp(timestamp, sizeof(timestamp));
    fprintf(log, "[%s] %s - %s\n", timestamp, event_type, filename);
    fclose(log);
}

// Mapea códigos de evento a texto legible
const char *get_event_name(uint32_t mask) {
    if (mask & IN_CREATE) return "CREADO";
    if (mask & IN_MODIFY) return "MODIFICADO";
    if (mask & IN_DELETE) return "ELIMINADO";
    if (mask & IN_MOVED_FROM) return "MOVIDO_DESDE";
    if (mask & IN_MOVED_TO) return "MOVIDO_A";
    return "OTRO_EVENTO";
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <directorio_a_monitorizar>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *path_to_watch = argv[1];

    int inotify_fd = inotify_init1(IN_NONBLOCK);
    if (inotify_fd < 0) {
        perror("inotify_init1");
        exit(EXIT_FAILURE);
    }

    int wd = inotify_add_watch(inotify_fd, path_to_watch,
                                IN_CREATE | IN_MODIFY | IN_DELETE |
                                IN_MOVED_FROM | IN_MOVED_TO);
    if (wd == -1) {
        perror("inotify_add_watch");
        close(inotify_fd);
        exit(EXIT_FAILURE);
    }

    printf("Monitorizando el directorio: %s\n", path_to_watch);
    printf("Presiona Ctrl+C para salir...\n");

    char buffer[BUF_LEN];
    while (1) {
        ssize_t length = read(inotify_fd, buffer, BUF_LEN);
        if (length < 0 && errno != EAGAIN) {
            perror("read");
            break;
        }

        if (length <= 0) {
            usleep(500 * 1000); // Espera breve (500ms)
            continue;
        }

        ssize_t i = 0;
        while (i < length) {
            struct inotify_event *event = (struct inotify_event *) &buffer[i];
            if (event->len) {
                const char *event_type = get_event_name(event->mask);
                log_event(event_type, event->name);
                printf("[%s] %s\n", event_type, event->name);
            }
            i += EVENT_SIZE + event->len;
        }
    }

    close(inotify_fd);
    return 0;
}
