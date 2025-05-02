#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sqlite3.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_LINE_LENGTH 1024
#define MAX_NAME_LENGTH 256
#define MAX_SKU_LENGTH 32
#define MAX_SQL_LENGTH 512

typedef struct {
    char nombre[MAX_NAME_LENGTH];
    double precio;
    char sku[MAX_SKU_LENGTH];
} Producto;

// Función segura para copiar cadenas
void safe_strcpy(char *dest, const char *src, size_t dest_size) {
    if (dest_size == 0) return;
    size_t len = strnlen(src, dest_size - 1);
    memcpy(dest, src, len);
    dest[len] = '\0';
}

int validar_sku(const char *sku) {
    if (strnlen(sku, MAX_SKU_LENGTH) != 9) return 0;
    
    for (int i = 0; i < 3; i++) {
        if (!isupper(sku[i])) return 0;
    }
    if (sku[3] != '-') return 0;
    for (int i = 4; i < 9; i++) {
        if (!isdigit(sku[i])) return 0;
    }
    return 1;
}

int procesar_linea_csv(const char *linea, Producto *producto) {
    char copia_linea[MAX_LINE_LENGTH + 1] = {0};
    safe_strcpy(copia_linea, linea, sizeof(copia_linea));

    char *token = strtok(copia_linea, ",");
    if (!token) return 0;
    safe_strcpy(producto->nombre, token, sizeof(producto->nombre));

    token = strtok(NULL, ",");
    if (!token) return 0;
    producto->precio = strtod(token, NULL);
    if (producto->precio <= 0) return 0;

    token = strtok(NULL, ",\n");
    if (!token) return 0;
    safe_strcpy(producto->sku, token, sizeof(producto->sku));
    
    return validar_sku(producto->sku);
}

int insertar_producto_seguro(sqlite3 *db, const Producto *producto) {
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO productos(nombre, precio, sku) VALUES(?, ?, ?);";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    
    sqlite3_bind_text(stmt, 1, producto->nombre, -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 2, producto->precio);
    sqlite3_bind_text(stmt, 3, producto->sku, -1, SQLITE_TRANSIENT);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

FILE* abrir_archivo_seguro(const char *nombre_archivo) {
    int fd = open(nombre_archivo, O_RDONLY | O_NOFOLLOW);
    if (fd == -1) return NULL;
    
    FILE *archivo = fdopen(fd, "r");
    if (!archivo) {
        close(fd);
        return NULL;
    }
    return archivo;
}

int procesar_csv(const char *nombre_archivo, const char *nombre_db) {
    FILE *archivo = abrir_archivo_seguro(nombre_archivo);
    if (!archivo) {
        perror("Error al abrir el archivo CSV");
        return 0;
    }

    sqlite3 *db;
    if (sqlite3_open_v2(nombre_db, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL) != SQLITE_OK) {
        fclose(archivo);
        return 0;
    }

    const char *sql_crear_tabla = 
        "CREATE TABLE IF NOT EXISTS productos("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "nombre TEXT NOT NULL,"
        "precio REAL NOT NULL,"
        "sku TEXT NOT NULL UNIQUE);";
    
    if (sqlite3_exec(db, sql_crear_tabla, NULL, NULL, NULL) != SQLITE_OK) {
        sqlite3_close(db);
        fclose(archivo);
        return 0;
    }

    char linea[MAX_LINE_LENGTH + 1] = {0};
    int lineas_procesadas = 0;
    int productos_insertados = 0;

    // Saltar cabecera
    if (!fgets(linea, sizeof(linea), archivo)) {
        sqlite3_close(db);
        fclose(archivo);
        return 0;
    }

    while (fgets(linea, sizeof(linea), archivo)) {
        lineas_procesadas++;
        linea[strcspn(linea, "\n")] = '\0';

        Producto producto = {0};
        if (procesar_linea_csv(linea, &producto)) {
            productos_insertados += insertar_producto_seguro(db, &producto);
        }
    }

    printf("Procesamiento completado. Líneas: %d, Productos insertados: %d\n", 
           lineas_procesadas, productos_insertados);

    fclose(archivo);
    sqlite3_close(db);
    return 1;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Uso: %s <archivo_csv> <base_datos>\n", argv[0]);
        return 1;
    }
    return procesar_csv(argv[1], argv[2]) ? 0 : 1;
}