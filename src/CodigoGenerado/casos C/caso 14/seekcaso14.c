#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_TAREAS 100
#define MAX_LONGITUD 256
#define ARCHIVO_TAREAS "tareas.dat"

typedef struct {
    int dia, mes, anio;
    int hora, minuto;
    char script[MAX_LONGITUD];
    int ejecutada; // 0 = pendiente, 1 = ejecutada
} Tarea;

Tarea tareas[MAX_TAREAS];
int num_tareas = 0;

void cargar_tareas() {
    FILE *archivo = fopen(ARCHIVO_TAREAS, "rb");
    if (archivo == NULL) return;

    num_tareas = 0;
    while (fread(&tareas[num_tareas], sizeof(Tarea), 1, archivo) {
        num_tareas++;
        if (num_tareas >= MAX_TAREAS) break;
    }

    fclose(archivo);
}

void guardar_tareas() {
    FILE *archivo = fopen(ARCHIVO_TAREAS, "wb");
    if (archivo == NULL) {
        perror("Error al guardar tareas");
        return;
    }

    for (int i = 0; i < num_tareas; i++) {
        fwrite(&tareas[i], sizeof(Tarea), 1, archivo);
    }

    fclose(archivo);
}

void agregar_tarea() {
    if (num_tareas >= MAX_TAREAS) {
        printf("No se pueden agregar más tareas. Límite alcanzado.\n");
        return;
    }

    Tarea nueva;
    printf("\nAgregar nueva tarea:\n");
    
    printf("Fecha (DD MM AAAA): ");
    scanf("%d %d %d", &nueva.dia, &nueva.mes, &nueva.anio);
    
    printf("Hora (HH MM): ");
    scanf("%d %d", &nueva.hora, &nueva.minuto);
    
    printf("Ruta del script a ejecutar: ");
    getchar(); // Limpiar el buffer
    fgets(nueva.script, MAX_LONGITUD, stdin);
    nueva.script[strcspn(nueva.script, "\n")] = '\0'; // Eliminar salto de línea
    
    nueva.ejecutada = 0;
    
    tareas[num_tareas++] = nueva;
    guardar_tareas();
    printf("Tarea agregada exitosamente.\n");
}

void listar_tareas() {
    printf("\nLista de tareas:\n");
    printf("ID | Fecha      | Hora  | Script               | Estado\n");
    printf("--------------------------------------------------------\n");
    
    for (int i = 0; i < num_tareas; i++) {
        printf("%2d | %02d/%02d/%04d | %02d:%02d | %-20s | %s\n",
               i,
               tareas[i].dia, tareas[i].mes, tareas[i].anio,
               tareas[i].hora, tareas[i].minuto,
               tareas[i].script,
               tareas[i].ejecutada ? "Ejecutada" : "Pendiente");
    }
}

void verificar_y_ejecutar_tareas() {
    time_t ahora = time(NULL);
    struct tm *tm_ahora = localtime(&ahora);
    
    for (int i = 0; i < num_tareas; i++) {
        if (tareas[i].ejecutada) continue;
        
        if (tareas[i].anio == tm_ahora->tm_year + 1900 &&
            tareas[i].mes == tm_ahora->tm_mon + 1 &&
            tareas[i].dia == tm_ahora->tm_mday &&
            tareas[i].hora == tm_ahora->tm_hour &&
            tareas[i].minuto == tm_ahora->tm_min) {
            
            printf("Ejecutando tarea programada: %s\n", tareas[i].script);
            
            pid_t pid = fork();
            if (pid == 0) {
                // Proceso hijo
                execl(tareas[i].script, tareas[i].script, NULL);
                perror("Error al ejecutar el script");
                exit(EXIT_FAILURE);
            } else if (pid > 0) {
                // Proceso padre
                int status;
                waitpid(pid, &status, 0);
                
                if (WIFEXITED(status)) {
                    printf("Script ejecutado con código de salida: %d\n", WEXITSTATUS(status));
                    tareas[i].ejecutada = 1;
                    guardar_tareas();
                }
            } else {
                perror("Error al crear proceso hijo");
            }
        }
    }
}

void menu() {
    int opcion;
    do {
        printf("\nSistema de Agendamiento de Tareas\n");
        printf("1. Agregar tarea\n");
        printf("2. Listar tareas\n");
        printf("3. Salir\n");
        printf("Seleccione una opcion: ");
        scanf("%d", &opcion);
        
        switch (opcion) {
            case 1:
                agregar_tarea();
                break;
            case 2:
                listar_tareas();
                break;
            case 3:
                printf("Saliendo...\n");
                break;
            default:
                printf("Opcion no valida.\n");
        }
    } while (opcion != 3);
}

int main() {
    cargar_tareas();
    verificar_y_ejecutar_tareas();
    menu();
    return 0;
}