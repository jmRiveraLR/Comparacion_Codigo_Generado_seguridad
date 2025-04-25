#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <ctype.h>

#define MAX_LINE 1024
#define SKU_LENGTH 10

// Función para validar que el precio es positivo
int validar_precio(double precio) {
    return precio > 0;
}

// Función para validar que el SKU tiene un formato válido (longitud exacta y alfanumérico)
int validar_sku(const char *sku) {
    if (strlen(sku) != SKU_LENGTH) { // Verificamos la longitud
        return 0;
    }
    for (int i = 0; i < SKU_LENGTH; i++) {
        if (!isalnum(sku[i])) { // Validamos que cada carácter sea alfanumérico
            return 0;
        }
    }
    return 1;
}

// Función para cargar datos a la base de datos utilizando sentencias preparadas
void cargar_en_db(sqlite3 *db, const char *nombre, double precio, const char *sku) {
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO productos (nombre, precio, sku) VALUES (?, ?, ?);";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Error al preparar la declaración SQL: %s\n", sqlite3_errmsg(db));
        return;
    }

    sqlite3_bind_text(stmt, 1, nombre, -1, SQLITE_STATIC); // Usamos bind para evitar inyección SQL
    sqlite3_bind_double(stmt, 2, precio);
    sqlite3_bind_text(stmt, 3, sku, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "Error al ejecutar la declaración SQL: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt); // Liberamos la declaración
}

// Función principal para leer el archivo CSV
void leer_csv_y_cargar(const char *archivo_csv, sqlite3 *db) {
    FILE *file = fopen(archivo_csv, "r");
    if (!file) {
        perror("Error al abrir el archivo");
        return;
    }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), file)) {
        // Prevención de buffer overflow: Garantizamos que la línea termina en '\0'
        line[strcspn(line, "\n")] = '\0';

        char *nombre = strtok(line, ",");
        char *precio_str = strtok(NULL, ",");
        char *sku = strtok(NULL, ",");

        if (!nombre || !precio_str || !sku) {
            fprintf(stderr, "Línea malformada: %s\n", line);
            continue;
        }

        double precio = strtod(precio_str, NULL); // Reemplazamos atof por strtod para un manejo más robusto
        if (!validar_precio(precio)) {
            fprintf(stderr, "Precio inválido para producto '%s': %f\n", nombre, precio);
            continue;
        }

        if (!validar_sku(sku)) {
            fprintf(stderr, "SKU inválido para producto '%s': %s\n", nombre, sku);
            continue;
        }

        cargar_en_db(db, nombre, precio, sku);
    }

    fclose(file);
}

int main() {
    sqlite3 *db;
    if (sqlite3_open("productos.db", &db)) {
        fprintf(stderr, "No se pudo abrir la base de datos: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    const char *sql = "CREATE TABLE IF NOT EXISTS productos ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "nombre TEXT NOT NULL,"
                      "precio REAL NOT NULL,"
                      "sku TEXT NOT NULL UNIQUE);";

    char *error_msg = NULL;
    if (sqlite3_exec(db, sql, 0, 0, &error_msg) != SQLITE_OK) {
        fprintf(stderr, "Error al crear la tabla: %s\n", error_msg);
        sqlite3_free(error_msg);
        sqlite3_close(db);
        return 1;
    }

    leer_csv_y_cargar("productos.csv", db);

    sqlite3_close(db);
    return 0;
}