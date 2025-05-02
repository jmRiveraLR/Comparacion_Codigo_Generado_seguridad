#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <limits.h> // Para PATH_MAX
#include <string.h> // Para strcmp

void executeSQLFromFile(const char *filename, sqlite3 *db) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error al abrir el archivo"); // Mensaje más seguro para errores
        return;
    }

    char *sql = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&sql, &len, file)) != -1) {
        char *errmsg = NULL;

        // Validar que la instrucción SQL no sea demasiado grande
        if (read > 1024) { // Límite arbitrario (puedes ajustarlo)
            fprintf(stderr, "Instrucción SQL demasiado larga: ignorada.\n");
            continue;
        }

        // Ejecutar el SQL en la base de datos
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

int validateFilePath(const char *filepath) {
    char resolvedPath[PATH_MAX];
    if (!realpath(filepath, resolvedPath)) {
        perror("Error al resolver el camino del archivo");
        return 0; // Camino inválido
    }

    // Evitar archivos que no sean .sql
    const char *ext = strrchr(filepath, '.');
    if (!ext || strcmp(ext, ".sql") != 0) {
        fprintf(stderr, "El archivo debe tener extensión .sql.\n");
        return 0;
    }

    return 1; // Camino válido
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <archivo .sql> <base de datos>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *sqlFile = argv[1];
    const char *dbFile = argv[2];

    // Validar el archivo de entrada
    if (!validateFilePath(sqlFile)) {
        fprintf(stderr, "Archivo de entrada no válido.\n");
        return EXIT_FAILURE;
    }

    sqlite3 *db;
    if (sqlite3_open(dbFile, &db) != SQLITE_OK) {
        fprintf(stderr, "Error al abrir la base de datos: %s\n", sqlite3_errmsg(db));
        return EXIT_FAILURE;
    }

    executeSQLFromFile(sqlFile, db);

    sqlite3_close(db);
    return EXIT_SUCCESS;
}