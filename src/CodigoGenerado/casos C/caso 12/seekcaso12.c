#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#define MAX_SQL_LENGTH 1024

// Función para procesar cada instrucción SQL
int execute_sql(sqlite3 *db, const char *sql) {
    char *err_msg = 0;
    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error al ejecutar SQL: %s\n", err_msg);
        sqlite3_free(err_msg);
        return 0;
    }
    
    return 1;
}

// Función para dividir el contenido del archivo en instrucciones SQL
void process_sql_file(sqlite3 *db, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "No se pudo abrir el archivo: %s\n", filename);
        return;
    }

    char buffer[MAX_SQL_LENGTH];
    char sql[MAX_SQL_LENGTH] = {0};
    int in_quote = 0;
    int in_comment = 0;
    int line_comment = 0;

    while (fgets(buffer, sizeof(buffer), file)) {
        // Procesar línea por línea
        for (int i = 0; buffer[i] != '\0'; i++) {
            // Manejar comentarios
            if (!in_quote && !in_comment && buffer[i] == '-' && buffer[i+1] == '-') {
                line_comment = 1;
                break;
            }
            
            if (!in_quote && !in_comment && buffer[i] == '/' && buffer[i+1] == '*') {
                in_comment = 1;
                i++;
                continue;
            }
            
            if (in_comment && buffer[i] == '*' && buffer[i+1] == '/') {
                in_comment = 0;
                i++;
                continue;
            }
            
            if (in_comment) continue;
            
            // Manejar comillas
            if (buffer[i] == '\'' && (i == 0 || buffer[i-1] != '\\')) {
                in_quote = !in_quote;
            }
            
            // Si no estamos en un comentario, agregar al SQL actual
            if (!line_comment) {
                strncat(sql, &buffer[i], 1);
            }
        }
        
        line_comment = 0;
        
        // Buscar punto y coma que no esté dentro de comillas
        if (!in_quote && strchr(buffer, ';') != NULL) {
            // Ejecutar la instrucción SQL
            if (strlen(sql) > 0) {
                printf("Ejecutando: %s\n", sql);
                if (!execute_sql(db, sql)) {
                    fprintf(stderr, "Error en la instrucción SQL\n");
                }
            }
            sql[0] = '\0'; // Resetear el buffer SQL
        }
    }
    
    // Ejecutar cualquier instrucción restante (sin punto y coma final)
    if (strlen(sql) > 0) {
        printf("Ejecutando (sin punto y coma final): %s\n", sql);
        execute_sql(db, sql);
    }
    
    fclose(file);
}

int main(int argc, char **argv) {
    sqlite3 *db;
    int rc;
    
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <archivo_bd> <archivo_sql>\n", argv[0]);
        return 1;
    }
    
    // Abrir la base de datos SQLite
    rc = sqlite3_open(argv[1], &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "No se pudo abrir la base de datos: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }
    
    // Procesar el archivo SQL
    process_sql_file(db, argv[2]);
    
    // Cerrar la base de datos
    sqlite3_close(db);
    
    printf("Proceso completado.\n");
    return 0;
}