#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sqlite3.h>

#define MAX_LINE_LENGTH 1024
#define MAX_NAME_LENGTH 256
#define MAX_SKU_LENGTH 32

// Estructura para almacenar los datos de un producto
typedef struct {
    char nombre[MAX_NAME_LENGTH];
    double precio;
    char sku[MAX_SKU_LENGTH];
} Producto;

// Función para validar el formato del SKU (ejemplo: ABC-12345)
int validar_sku(const char *sku) {
    int i = 0;
    
    // Los primeros 3 caracteres deben ser letras mayúsculas
    for (; i < 3; i++) {
        if (!isupper(sku[i])) {
            return 0;
        }
    }
    
    // Debe haber un guión
    if (sku[i++] != '-') {
        return 0;
    }
    
    // Los siguientes 5 caracteres deben ser dígitos
    for (; i < 9; i++) {
        if (!isdigit(sku[i])) {
            return 0;
        }
    }
    
    // El SKU debe terminar aquí
    if (sku[i] != '\0') {
        return 0;
    }
    
    return 1;
}

// Función para procesar una línea del CSV
int procesar_linea_csv(const char *linea, Producto *producto) {
    char *token;
    char copia_linea[MAX_LINE_LENGTH];
    
    // Hacer una copia para no modificar la línea original
    strncpy(copia_linea, linea, MAX_LINE_LENGTH);
    
    // Extraer nombre (primer campo)
    token = strtok(copia_linea, ",");
    if (token == NULL) return 0;
    strncpy(producto->nombre, token, MAX_NAME_LENGTH);
    
    // Extraer precio (segundo campo)
    token = strtok(NULL, ",");
    if (token == NULL) return 0;
    producto->precio = atof(token);
    if (producto->precio <= 0) {
        fprintf(stderr, "Error: Precio no positivo: %s\n", token);
        return 0;
    }
    
    // Extraer SKU (tercer campo)
    token = strtok(NULL, ",\n");
    if (token == NULL) return 0;
    strncpy(producto->sku, token, MAX_SKU_LENGTH);
    if (!validar_sku(producto->sku)) {
        fprintf(stderr, "Error: Formato de SKU inválido: %s\n", producto->sku);
        return 0;
    }
    
    return 1;
}

// Función callback para SQLite
static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
    return 0;
}

// Función para insertar un producto en la base de datos
int insertar_producto(sqlite3 *db, const Producto *producto) {
    char *err_msg = 0;
    char sql[512];
    
    snprintf(sql, sizeof(sql), 
             "INSERT INTO productos(nombre, precio, sku) VALUES('%s', %f, '%s');",
             producto->nombre, producto->precio, producto->sku);
    
    int rc = sqlite3_exec(db, sql, callback, 0, &err_msg);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error SQL: %s\n", err_msg);
        sqlite3_free(err_msg);
        return 0;
    }
    
    return 1;
}

// Función principal para procesar el archivo CSV
int procesar_csv(const char *nombre_archivo, const char *nombre_db) {
    FILE *archivo = fopen(nombre_archivo, "r");
    if (!archivo) {
        perror("Error al abrir el archivo CSV");
        return 0;
    }
    
    sqlite3 *db;
    int rc = sqlite3_open(nombre_db, &db);
    if (rc) {
        fprintf(stderr, "Error al abrir la base de datos: %s\n", sqlite3_errmsg(db));
        fclose(archivo);
        return 0;
    }
    
    // Crear tabla si no existe
    char *err_msg = 0;
    const char *sql_crear_tabla = 
        "CREATE TABLE IF NOT EXISTS productos("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "nombre TEXT NOT NULL,"
        "precio REAL NOT NULL,"
        "sku TEXT NOT NULL UNIQUE);";
    
    rc = sqlite3_exec(db, sql_crear_tabla, callback, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error al crear tabla: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        fclose(archivo);
        return 0;
    }
    
    char linea[MAX_LINE_LENGTH];
    int lineas_procesadas = 0;
    int productos_insertados = 0;
    
    // Saltar la cabecera si existe
    fgets(linea, sizeof(linea), archivo);
    
    while (fgets(linea, sizeof(linea), archivo)) {
        lineas_procesadas++;
        
        // Eliminar el salto de línea al final
        linea[strcspn(linea, "\n")] = 0;
        
        Producto producto;
        if (procesar_linea_csv(linea, &producto)) {
            if (insertar_producto(db, &producto)) {
                productos_insertados++;
            } else {
                fprintf(stderr, "Error al insertar producto de la línea %d\n", lineas_procesadas);
            }
        } else {
            fprintf(stderr, "Error al procesar línea %d: %s\n", lineas_procesadas, linea);
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
    
    if (!procesar_csv(argv[1], argv[2])) {
        return 1;
    }
    
    return 0;
}