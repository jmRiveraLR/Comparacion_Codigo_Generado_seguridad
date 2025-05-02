#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_LINEA 512
#define ARCHIVO_TAREAS "tareas.txt"

typedef struct {
    int anio, mes, dia, hora, minuto;
    char ruta[256];
} Tarea;

void ejecutar_tarea(const Tarea* t) {
    printf("Ejecutando tarea programada: %s\n", t->ruta);
    system(t->ruta);
}

time_t tarea_a_epoch(const Tarea* t) {
    struct tm tm_fecha = {0};
    tm_fecha.tm_year = t->anio - 1900;
    tm_fecha.tm_mon  = t->mes - 1;
    tm_fecha.tm_mday = t->dia;
    tm_fecha.tm_hour = t->hora;
    tm_fecha.tm_min  = t->minuto;
    return mktime(&tm_fecha);
}

int tarea_vencida(const Tarea* t) {
    time_t ahora = time(NULL);
    return difftime(ahora, tarea_a_epoch(t)) >= 0;
}

void revisar_y_ejecutar() {
    FILE* f = fopen(ARCHIVO_TAREAS, "r");
    if (!f) return;

    FILE* temp = fopen("temp.txt", "w");
    Tarea t;

    while (fscanf(f, "%d-%d-%d %d:%d %[^\n]", &t.anio, &t.mes, &t.dia, &t.hora, &t.minuto, t.ruta) == 6) {
        if (tarea_vencida(&t)) {
            ejecutar_tarea(&t);
        } else {
            fprintf(temp, "%04d-%02d-%02d %02d:%02d %s\n", t.anio, t.mes, t.dia, t.hora, t.minuto, t.ruta);
        }
    }

    fclose(f);
    fclose(temp);
    remove(ARCHIVO_TAREAS);
    rename("temp.txt", ARCHIVO_TAREAS);
}

void agregar_tarea() {
    Tarea t;
    printf("Fecha (AAAA-MM-DD): ");
    scanf("%d-%d-%d", &t.anio, &t.mes, &t.dia);
    printf("Hora (HH:MM): ");
    scanf("%d:%d", &t.hora, &t.minuto);
    printf("Ruta del script a ejecutar: ");
    getchar(); // limpiar buffer
    fgets(t.ruta, sizeof(t.ruta), stdin);
    t.ruta[strcspn(t.ruta, "\n")] = 0; // eliminar salto de línea

    FILE* f = fopen(ARCHIVO_TAREAS, "a");
    if (!f) {
        perror("No se pudo abrir archivo de tareas");
        return;
    }

    fprintf(f, "%04d-%02d-%02d %02d:%02d %s\n", t.anio, t.mes, t.dia, t.hora, t.minuto, t.ruta);
    fclose(f);
    printf("Tarea registrada.\n");
}

int main() {
    revisar_y_ejecutar();

    int opcion;
    printf("\n1. Agregar nueva tarea\n0. Salir\nOpción: ");
    scanf("%d", &opcion);
    if (opcion == 1) {
        agregar_tarea();
    }

    return 0;
}
