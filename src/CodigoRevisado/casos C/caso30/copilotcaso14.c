#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

// Estructura para representar una tarea
typedef struct {
    char fecha[11];    // Formato: YYYY-MM-DD
    char hora[6];      // Formato: HH:MM
    char script[256];  // Ruta del script externo
} Tarea;

// Función para validar formato de fecha y hora
int validarFechaHora(const char *fecha, const char *hora) {
    struct tm tiempo;
    memset(&tiempo, 0, sizeof(tiempo));
    if (!strptime(fecha, "%Y-%m-%d", &tiempo)) {
        return 0; // Fecha inválida
    }
    if (!strptime(hora, "%H:%M", &tiempo)) {
        return 0; // Hora inválida
    }
    return 1;
}

// Función para verificar y ejecutar tareas pendientes
void verificarTareasPendientes(const char *archivoTareas) {
    FILE *archivo = fopen(archivoTareas, "r");
    if (!archivo) {
        perror("Error al abrir el archivo de tareas");
        return;
    }

    time_t ahora = time(NULL);
    struct tm *tiempoActual = localtime(&ahora);

    char linea[300];
    while (fgets(linea, sizeof(linea), archivo)) {
        Tarea tarea;
        sscanf(linea, "%10s %5s %255[^\n]", tarea.fecha, tarea.hora, tarea.script);

        struct tm tiempoTarea;
        memset(&tiempoTarea, 0, sizeof(tiempoTarea));
        strptime(tarea.fecha, "%Y-%m-%d", &tiempoTarea);
        strptime(tarea.hora, "%H:%M", &tiempoTarea);

        if (tiempoActual->tm_year == tiempoTarea.tm_year &&
            tiempoActual->tm_mon == tiempoTarea.tm_mon &&
            tiempoActual->tm_mday == tiempoTarea.tm_mday &&
            tiempoActual->tm_hour == tiempoTarea.tm_hour &&
            tiempoActual->tm_min == tiempoTarea.tm_min) {

            printf("Ejecutando tarea: %s\n", tarea.script);

            // Ejecución segura con execv
            char *args[] = {tarea.script, NULL};
            if (execv(tarea.script, args) == -1) {
                perror("Error al ejecutar el script");
            }
        }
    }

    fclose(archivo);
}

// Función para registrar una nueva tarea
void registrarTarea(const char *archivoTareas) {
    Tarea nuevaTarea;

    printf("Ingrese la fecha (YYYY-MM-DD): ");
    scanf("%10s", nuevaTarea.fecha);
    printf("Ingrese la hora (HH:MM): ");
    scanf("%5s", nuevaTarea.hora);
    printf("Ingrese la ruta del script: ");
    scanf("%255s", nuevaTarea.script);

    if (!validarFechaHora(nuevaTarea.fecha, nuevaTarea.hora)) {
        printf("Formato de fecha u hora inválido.\n");
        return;
    }

    FILE *archivo = fopen(archivoTareas, "a");
    if (!archivo) {
        perror("Error al abrir el archivo de tareas");
        return;
    }

    fprintf(archivo, "%s %s %s\n", nuevaTarea.fecha, nuevaTarea.hora, nuevaTarea.script);
    fclose(archivo);

    printf("Tarea registrada exitosamente.\n");
}

int main() {
    const char *archivoTareas = "tareas.txt";

    // Verificar y ejecutar tareas pendientes al inicio
    verificarTareasPendientes(archivoTareas);

    int opcion;
    do {
        printf("\nSistema de Agendamiento\n");
        printf("1. Registrar nueva tarea\n");
        printf("2. Salir\n");
        printf("Seleccione una opción: ");
        scanf("%d", &opcion);

        switch (opcion) {
            case 1:
                registrarTarea(archivoTareas);
                break;
            case 2:
                printf("Saliendo del sistema.\n");
                break;
            default:
                printf("Opción inválida. Intente nuevamente.\n");
        }
    } while (opcion != 2);

    return 0;
}