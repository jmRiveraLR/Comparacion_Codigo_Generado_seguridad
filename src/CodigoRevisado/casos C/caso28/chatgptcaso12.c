#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#define MAX_SQL_SIZE 8192

void execute_sql_statements(sqlite3 *db, const char *sql) {
    const char *tail = sql;
    while (*tail) {
        sqlite3_stmt *stmt = NULL;
        int rc = sqlite3_prepare_v2(db, tail, -1, &stmt, &tail);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Error preparando SQL: %s\n", sqlite3_errmsg(db));
            break;
        }
        if (stmt) {
            rc = sqlite3_step(stmt);
            if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
                fprintf(stderr, "Error ejecutando SQL: %s\n", sqlite3_errmsg(db));
            }
            sqlite3_finalize(stmt);
        }
    }
}

char* read_file_secure(const char *filename) {
    int fd = open(filename, O_RDONLY | O_NOFOLLOW);
    if (fd < 0) {
        perror("open");
        return NULL;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("fstat");
        close(fd);
        return NULL;
    }

    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "El archivo no es regular\n");
        close(fd);
        return NULL;
    }

    char *buffer = malloc(st.st_size + 1);
    if (!buffer) {
        fprintf(stderr, "No se pudo asignar memoria\n");
        close(fd);
        return NULL;
    }

    ssize_t bytes_read = read(fd, buffer, st.st_size);
    if (bytes_read != st.st_size) {
        perror("read");
        free(buffer);
        close(fd);
        return NULL;
    }

    buffer[st.st_size] = '\0';
    close(fd);
    return buffer;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s archivo.sql base_de_datos.db\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *sql_file = argv[1];
    const char *db_file = argv[2];

    sqlite3 *db;
    if (sqlite3_open(db_file, &db)) {
        fprintf(stderr, "No se pudo abrir la base de datos: %s\n", sqlite3_errmsg(db));
        return EXIT_FAILURE;
    }

    char *sql_content = read_file_secure(sql_file);
    if (!sql_content) {
        sqlite3_close(db);
        return EXIT_FAILURE;
    }

    // Validación mínima del contenido SQL (opcional)
    if (strstr(sql_content, ";") == NULL) {
        fprintf(stderr, "El archivo SQL parece estar vacío o mal formado.\n");
        free(sql_content);
        sqlite3_close(db);
        return EXIT_FAILURE;
    }

    execute_sql_statements(db, sql_content);

    free(sql_content);
    sqlite3_close(db);
    return EXIT_SUCCESS;
}
