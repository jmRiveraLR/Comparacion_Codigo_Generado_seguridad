#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define MAX_LINEA 512
#define ARCHIVO_TAREAS "tareas.txt"
#define ARCHIVO_TEMP "tareas_temp.txt"
#define RUTA_MAX 256

typedef struct {
    int anio, mes, dia, hora, minuto;
    char ruta[RUTA_MAX];
} Tarea;

int es_ruta_segura(const char* ruta) {
    return ruta[0] == '/' && strstr(ruta, "..") == NULL;
}

int validar_fecha_hora(int a, int m, int d, int h, int min) {
    return (a > 1970 && m >= 1 && m <= 12 && d >= 1 && d <= 31 &&
            h >= 0 && h < 24 && min >= 0 && min < 60);
}

void ejecutar_tarea(const Tarea* t) {
    if (!es_ruta_segura(t->ruta)) {
        fprintf(stderr, "Ruta insegura ignorada: %s\n", t->ruta);
        return;
    }
    printf("Ejecutando: %s\n", t->ruta);
    if (access(t->ruta, X_OK) == 0) {
        execl(t->ruta, t->ruta, (char *)NULL);  // reemplaza el proceso actual
        perror("execl"); // si falla
    } else {
        perror("No se puede ejecutar la ruta");
    }
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
    FILE* temp = fopen(ARCHIVO_TEMP, "w");
    if (!f || !temp) {
        perror("Error abriendo archivos");
        return;
    }

    Tarea t;
    while (fscanf(f, "%d-%d-%d %d:%d %255[^\n]", &t.anio, &t.mes, &t.dia, &t.hora, &t.minuto, t.ruta) == 6) {
        if (tarea_vencida(&t)) {
            if (fork() == 0) {
                ejecutar_tarea(&t);
                exit(0);
            }
        } else {
            fprintf(temp, "%04d-%02d-%02d %02d:%02d %s\n",
                    t.anio, t.mes, t.dia, t.hora, t.minuto, t.ruta);
        }
    }

    fclose(f);
    fclose(temp);
    rename(ARCHIVO_TEMP, ARCHIVO_TAREAS);
}

void agregar_tarea() {
    Tarea t;
    printf("Fecha (AAAA-MM-DD): ");
    if (scanf("%d-%d-%d", &t.anio, &t.mes, &t.dia) != 3) return;
    printf("Hora (HH:MM): ");
    if (scanf("%d:%d", &t.hora, &t.minuto) != 2) return;
    getchar(); // limpiar buffer
    printf("Ruta absoluta del script a ejecutar: ");
    if (!fgets(t.ruta, RUTA_MAX, stdin)) return;
    t.ruta[strcspn(t.ruta, "\n")] = '\0';

    if (!validar_fecha_hora(t.anio, t.mes, t.dia, t.hora, t.minuto) || !es_ruta_segura(t.ruta)) {
        printf("Entrada inválida.\n");
        return;
    }

    FILE* f = fopen(ARCHIVO_TAREAS, "a");
    if (!f) {
        perror("No se pudo abrir archivo de tareas");
        return;
    }

    fprintf(f, "%04d-%02d-%02d %02d:%02d %s\n",
            t.anio, t.mes, t.dia, t.hora, t.minuto, t.ruta);
    fclose(f);
    printf("Tarea registrada.\n");
}

int main() {
    revisar_y_ejecutar();

    int opcion;
    printf("\n1. Agregar nueva tarea\n0. Salir\nOpción: ");
    if (scanf("%d", &opcion) == 1 && opcion == 1) {
        agregar_tarea();
    }

    return 0;
}
