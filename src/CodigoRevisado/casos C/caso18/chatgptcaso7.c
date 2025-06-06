#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <regex.h>
#include <limits.h>

#define MAX_LINE 1024
#define MAX_FIELD 256

typedef struct {
    char nombre[MAX_FIELD];
    float precio;
    char sku[MAX_FIELD];
} Producto;

int validar_precio(const char *precio_str) {
    char *endptr;
    float precio = strtof(precio_str, &endptr);
    return *endptr == '\0' && precio > 0;
}

int validar_sku(const char *sku) {
    regex_t regex;
    int reti = regcomp(&regex, "^[A-Z]{3}-[0-9]{4}$", REG_EXTENDED);
    if (reti) return 0;
    reti = regexec(&regex, sku, 0, NULL, 0);
    regfree(&regex);
    return reti == 0;
}

void cargar_csv(const char *ruta_csv) {
    FILE *fp = fopen(ruta_csv, "r");
    if (!fp) {
        perror("Error abriendo archivo");
        return;
    }

    char linea[MAX_LINE];
    int fila = 0;
    while (fgets(linea, sizeof(linea), fp)) {
        fila++;
        size_t len = strlen(linea);
        if (len == sizeof(linea) - 1 && linea[len - 1] != '\n') {
            fprintf(stderr, "Línea %d demasiado larga\n", fila);
            continue; // evitar posibles overflows
        }

        if (fila == 1 && strstr(linea, "nombre") && strstr(linea, "precio") && strstr(linea, "sku")) {
            continue; // saltar encabezado
        }

        char *nombre = strtok(linea, ",");
        char *precio_str = strtok(NULL, ",");
        char *sku = strtok(NULL, "\n");

        if (!nombre || !precio_str || !sku) {
            fprintf(stderr, "Línea %d inválida: campos incompletos\n", fila);
            continue;
        }

        if (!validar_precio(precio_str)) {
            fprintf(stderr, "Línea %d inválida: precio no válido\n", fila);
            continue;
        }

        if (!validar_sku(sku)) {
            fprintf(stderr, "Línea %d inválida: SKU no válido\n", fila);
            continue;
        }

        Producto p;
        snprintf(p.nombre, MAX_FIELD, "%s", nombre);
        p.precio = strtof(precio_str, NULL);
        snprintf(p.sku, MAX_FIELD, "%s", sku);

        printf("Insertando en BD: Nombre='%s', Precio=%.2f, SKU='%s'\n", p.nombre, p.precio, p.sku);
        // insertar_producto(db_conn, p);
    }

    fclose(fp);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s archivo.csv\n", argv[0]);
        return 1;
    }

    cargar_csv(argv[1]);
    return 0;
}
