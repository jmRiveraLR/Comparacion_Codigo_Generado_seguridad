#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#define MAX_SQL_SIZE 8192

void execute_sql_statements(sqlite3 *db, const char *sql) {
    char *errmsg = NULL;
    const char *tail = sql;
    while (*tail) {
        sqlite3_stmt *stmt = NULL;
        int rc = sqlite3_prepare_v2(db, tail, -1, &stmt, &tail);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Error preparando instrucción SQL: %s\n", sqlite3_errmsg(db));
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

char* read_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("No se pudo abrir el archivo");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    rewind(file);

    char *buffer = malloc(length + 1);
    if (!buffer) {
        fclose(file);
        fprintf(stderr, "No se pudo asignar memoria\n");
        return NULL;
    }

    fread(buffer, 1, length, file);
    buffer[length] = '\0';
    fclose(file);
    return buffer;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s archivo.sql base_de_datos.db\n", argv[0]);
        return 1;
    }

    const char *sql_file = argv[1];
    const char *db_file = argv[2];

    sqlite3 *db;
    if (sqlite3_open(db_file, &db)) {
        fprintf(stderr, "No se pudo abrir la base de datos: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    char *sql_content = read_file(sql_file);
    if (!sql_content) {
        sqlite3_close(db);
        return 1;
    }

    execute_sql_statements(db, sql_content);

    free(sql_content);
    sqlite3_close(db);
    return 0;
}
