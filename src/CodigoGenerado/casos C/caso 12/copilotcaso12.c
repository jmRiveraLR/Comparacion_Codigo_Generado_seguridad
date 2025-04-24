#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>

void executeSQLFromFile(const char *filename, sqlite3 *db) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error al abrir el archivo %s\n", filename);
        return;
    }

    char *sql = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&sql, &len, file)) != -1) {
        char *errmsg = NULL;
        if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
            fprintf(stderr, "Error al ejecutar la instrucción SQL: %s\n", errmsg);
            sqlite3_free(errmsg);
        } else {
            printf("Instrucción ejecutada exitosamente: %s", sql);
        }
    }

    free(sql);
    fclose(file);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <nombre de archivo .sql> <nombre de la base de datos>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *sqlFile = argv[1];
    const char *dbFile = argv[2];

    sqlite3 *db;
    if (sqlite3_open(dbFile, &db) != SQLITE_OK) {
        fprintf(stderr, "Error al abrir la base de datos: %s\n", sqlite3_errmsg(db));
        return EXIT_FAILURE;
    }

    executeSQLFromFile(sqlFile, db);

    sqlite3_close(db);
    return EXIT_SUCCESS;
}