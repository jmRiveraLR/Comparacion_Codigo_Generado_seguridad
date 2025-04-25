#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <limits.h>
#include <sys/stat.h>

#define INITIAL_SQL_CAPACITY 4096
#define MAX_SQL_LENGTH (10 * 1024 * 1024) // 10MB como límite máximo

typedef struct {
    char *data;
    size_t size;
    size_t capacity;
} DynamicString;

// Inicializa una cadena dinámica
void init_dynamic_string(DynamicString *ds) {
    ds->data = malloc(INITIAL_SQL_CAPACITY);
    if (!ds->data) {
        perror("Error de asignación de memoria");
        exit(EXIT_FAILURE);
    }
    ds->data[0] = '\0';
    ds->size = 0;
    ds->capacity = INITIAL_SQL_CAPACITY;
}

// Añade un carácter a la cadena dinámica de forma segura
void safe_append_char(DynamicString *ds, char c) {
    if (ds->size + 1 >= ds->capacity) {
        size_t new_capacity = ds->capacity * 2;
        if (new_capacity > MAX_SQL_LENGTH) {
            fprintf(stderr, "Límite máximo de tamaño SQL alcanzado\n");
            exit(EXIT_FAILURE);
        }
        
        char *new_data = realloc(ds->data, new_capacity);
        if (!new_data) {
            perror("Error de reasignación de memoria");
            exit(EXIT_FAILURE);
        }
        
        ds->data = new_data;
        ds->capacity = new_capacity;
    }
    
    ds->data[ds->size++] = c;
    ds->data[ds->size] = '\0';
}

// Verifica si el archivo es regular y seguro para abrir
int is_regular_file(const char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat) != 0) {
        return 0;
    }
    return S_ISREG(path_stat.st_mode);
}

// Función para ejecutar SQL con manejo seguro de errores
int execute_sql(sqlite3 *db, const char *sql) {
    if (!sql || *sql == '\0') return 1;
    
    char *err_msg = NULL;
    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error SQL: %s\nConsulta: %.*s\n", 
                err_msg, 100, sql); // Limitar salida a 100 caracteres
        sqlite3_free(err_msg);
        return 0;
    }
    
    return 1;
}

// Procesa el archivo SQL de forma segura
void process_sql_file(sqlite3 *db, const char *filename) {
    if (!is_regular_file(filename)) {
        fprintf(stderr, "Ruta no válida o no es un archivo regular: %s\n", filename);
        return;
    }

    FILE *file = fopen(filename, "rb"); // Modo binario para mayor portabilidad
    if (!file) {
        perror("Error al abrir el archivo SQL");
        return;
    }

    DynamicString sql;
    init_dynamic_string(&sql);

    int in_quote = 0, in_comment = 0, line_comment = 0;
    int c;

    while ((c = fgetc(file)) != EOF) {
        if (c == '\0') break; // Carácter nulo - posible archivo binario

        // Manejar comentarios
        if (!in_quote && !in_comment && c == '-' && (c = fgetc(file)) == '-') {
            line_comment = 1;
            continue;
        }
        
        if (!in_quote && !in_comment && c == '/' && (c = fgetc(file)) == '*') {
            in_comment = 1;
            continue;
        }
        
        if (in_comment && c == '*' && (c = fgetc(file)) == '/') {
            in_comment = 0;
            continue;
        }
        
        if (in_comment || line_comment) {
            if (c == '\n') line_comment = 0;
            continue;
        }
        
        // Manejar comillas
        if (c == '\'' && (sql.size == 0 || sql.data[sql.size-1] != '\\')) {
            in_quote = !in_quote;
        }
        
        // Agregar carácter al SQL actual
        safe_append_char(&sql, c);
        
        // Buscar fin de instrucción (punto y coma fuera de comillas)
        if (!in_quote && c == ';') {
            if (sql.size > 1) { // Ignorar solo ";"
                execute_sql(db, sql.data);
            }
            free(sql.data);
            init_dynamic_string(&sql);
        }
    }

    // Ejecutar cualquier instrucción restante
    if (sql.size > 0 && !line_comment && !in_comment) {
        execute_sql(db, sql.data);
    }

    free(sql.data);
    fclose(file);
}

int main(int argc, char **argv) {
    sqlite3 *db;
    int rc;
    
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <archivo_bd> <archivo_sql>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    // Validar ruta de la base de datos
    if (!is_regular_file(argv[1]) && strcmp(argv[1], ":memory:") != 0) {
        fprintf(stderr, "Ruta de base de datos no válida: %s\n", argv[1]);
        return EXIT_FAILURE;
    }
    
    // Abrir base de datos SQLite
    rc = sqlite3_open_v2(argv[1], &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "No se pudo abrir la base de datos: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return EXIT_FAILURE;
    }
    
    // Habilitar características de seguridad
    sqlite3_limit(db, SQLITE_LIMIT_LENGTH, MAX_SQL_LENGTH);
    sqlite3_limit(db, SQLITE_LIMIT_SQL_LENGTH, MAX_SQL_LENGTH);
    
    // Procesar archivo SQL
    process_sql_file(db, argv[2]);
    
    // Cerrar base de datos
    sqlite3_close(db);
    
    return EXIT_SUCCESS;
}