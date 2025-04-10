#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16))

void log_event(const char *log_file, const char *event_desc, const char *file_name) {
    FILE *log = fopen(log_file, "a");
    if (log == NULL) {
        perror("Error abriendo el archivo de log");
        exit(EXIT_FAILURE);
    }

    time_t now = time(NULL);
    struct tm *timeinfo = localtime(&now);

    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timeinfo);

    fprintf(log, "[%s] %s: %s\n", time_str, event_desc, file_name);
    fclose(log);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <directorio_a_monitorizar> <archivo_de_log>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *directory = argv[1];
    const char *log_file = argv[2];
    int inotify_fd = inotify_init();

    if (inotify_fd < 0) {
        perror("Error inicializando inotify");
        exit(EXIT_FAILURE);
    }

    int watch_descriptor = inotify_add_watch(inotify_fd, directory, IN_CREATE | IN_MODIFY | IN_DELETE);
    if (watch_descriptor < 0) {
        perror("Error añadiendo watch");
        exit(EXIT_FAILURE);
    }

    char buffer[EVENT_BUF_LEN];

    printf("Monitorizando cambios en el directorio: %s\n", directory);

    while (1) {
        int length = read(inotify_fd, buffer, EVENT_BUF_LEN);
        if (length < 0) {
            perror("Error leyendo eventos");
            exit(EXIT_FAILURE);
        }

        int i = 0;
        while (i < length) {
            struct inotify_event *event = (struct inotify_event *)&buffer[i];

            if (event->len) {
                if (event->mask & IN_CREATE) {
                    log_event(log_file, "CREACIÓN", event->name);
                } else if (event->mask & IN_MODIFY) {
                    log_event(log_file, "MODIFICACIÓN", event->name);
                } else if (event->mask & IN_DELETE) {
                    log_event(log_file, "ELIMINACIÓN", event->name);
                }
            }

            i += EVENT_SIZE + event->len;
        }
    }

    inotify_rm_watch(inotify_fd, watch_descriptor);
    close(inotify_fd);
    return 0;
}