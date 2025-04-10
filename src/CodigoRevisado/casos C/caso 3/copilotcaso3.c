#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16))

void log_event(const char *log_file, const char *event_desc, const char *file_name) {
    int fd = open(log_file, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        perror("Error al abrir el archivo de log");
        exit(EXIT_FAILURE);
    }

    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    if (fcntl(fd, F_SETLK, &lock) == -1) {
        perror("Error al bloquear el archivo de log");
        close(fd);
        exit(EXIT_FAILURE);
    }

    time_t now = time(NULL);
    struct tm *timeinfo = localtime(&now);

    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timeinfo);

    char log_entry[256];
    snprintf(log_entry, sizeof(log_entry), "[%s] %s: %s\n", time_str, event_desc, file_name);

    if (write(fd, log_entry, strlen(log_entry)) < 0) {
        perror("Error al escribir en el archivo de log");
    }

    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);

    close(fd);
}

void validate_path(const char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat) != 0 || !S_ISDIR(path_stat.st_mode)) {
        fprintf(stderr, "El directorio especificado no es válido o no existe.\n");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <directorio_a_monitorizar> <archivo_de_log>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *directory = argv[1];
    const char *log_file = argv[2];

    validate_path(directory);

    int inotify_fd = inotify_init1(IN_NONBLOCK);
    if (inotify_fd < 0) {
        perror("Error inicializando inotify");
        exit(EXIT_FAILURE);
    }

    int watch_descriptor = inotify_add_watch(inotify_fd, directory, IN_CREATE | IN_MODIFY | IN_DELETE);
    if (watch_descriptor < 0) {
        perror("Error añadiendo watch");
        close(inotify_fd);
        exit(EXIT_FAILURE);
    }

    char buffer[EVENT_BUF_LEN];

    printf("Monitorizando cambios en el directorio: %s\n", directory);

    while (1) {
        int length = read(inotify_fd, buffer, EVENT_BUF_LEN);
        if (length < 0 && errno != EAGAIN) {
            perror("Error leyendo eventos");
            break;
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