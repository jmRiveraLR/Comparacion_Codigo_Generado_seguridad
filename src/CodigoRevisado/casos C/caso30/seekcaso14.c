#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <ctype.h>

#define MAX_TAREAS 100
#define MAX_LONGITUD 256
#define ARCHIVO_TAREAS "/var/lib/task-scheduler/tareas.dat"
#define ALLOWED_SCRIPT_DIR "/usr/local/bin/approved-scripts/"

typedef struct {
    int dia, mes, anio;
    int hora, minuto;
    char script[MAX_LONGITUD];
    int ejecutada;
} Tarea;

Tarea tareas[MAX_TAREAS];
int num_tareas = 0;

// Función para validar rutas seguras
int es_ruta_segura(const char *ruta) {
    // Verificar que comience con el directorio permitido
    if (strncmp(ruta, ALLOWED_SCRIPT_DIR, strlen(ALLOWED_SCRIPT_DIR)) != 0) {
        return 0;
    }

    // Verificar caracteres peligrosos
    const char *caracteres_peligrosos = ";&|<>$(){}[]'\"\n\r\t";
    for (const char *c = caracteres_peligrosos; *c; c++) {
        if (strchr(ruta, *c) != NULL) {
            return 0;
        }
    }

    // Verificar componentes de ruta
    if (strstr(ruta, "../") || strstr(ruta, "/..") || strstr(ruta, "//")) {
        return 0;
    }

    return 1;
}

// Función para verificar existencia y permisos del archivo
int archivo_es_seguro(const char *ruta) {
    struct stat statbuf;
    
    if (stat(ruta, &statbuf) {
        perror("Error al verificar archivo");
        return 0;
    }

    // Verificar que no sea un enlace simbólico
    if (S_ISLNK(statbuf.st_mode)) {
        fprintf(stderr, "Error: El archivo es un enlace simbólico\n");
        return 0;
    }

    // Verificar permisos (solo el propietario puede escribir)
    if (statbuf.st_mode & S_IWGRP || statbuf.st_mode & S_IWOTH) {
        fprintf(stderr, "Error: Permisos de escritura demasiado abiertos\n");
        return 0;
    }

    return 1;
}

void cargar_tareas() {
    FILE *archivo = fopen(ARCHIVO_TAREAS, "rb");
    if (archivo == NULL) {
        // Crear directorio si no existe
        mkdir("/var/lib/task-scheduler", 0755);
        return;
    }

    // Bloquear el archivo para evitar condiciones de carrera
    flock(fileno(archivo), LOCK_EX);

    num_tareas = 0;
    while (fread(&tareas[num_tareas], sizeof(Tarea), 1, archivo) == 1) {
        // Validar cada tarea cargada
        if (es_ruta_segura(tareas[num_tareas].script)) {
            num_tareas++;
            if (num_tareas >= MAX_TAREAS) break;
        }
    }

    flock(fileno(archivo), LOCK_UN);
    fclose(archivo);
}

void guardar_tareas() {
    // Crear archivo temporal de forma segura
    char temp_path[] = "/var/lib/task-scheduler/temp.XXXXXX";
    int fd = mkstemp(temp_path);
    if (fd == -1) {
        perror("Error al crear archivo temporal");
        return;
    }

    FILE *archivo = fdopen(fd, "wb");
    if (archivo == NULL) {
        close(fd);
        unlink(temp_path);
        perror("Error al abrir archivo temporal");
        return;
    }

    // Bloquear el archivo
    flock(fd, LOCK_EX);

    for (int i = 0; i < num_tareas; i++) {
        if (fwrite(&tareas[i], sizeof(Tarea), 1, archivo) != 1) {
            perror("Error al escribir tarea");
            break;
        }
    }

    // Asegurar que los datos se escriben
    fflush(archivo);
    flock(fd, LOCK_UN);
    fclose(archivo);

    // Reemplazar archivo original de forma atómica
    if (rename(temp_path, ARCHIVO_TAREAS) != 0) {
        perror("Error al renombrar archivo temporal");
        unlink(temp_path);
    }

    // Establecer permisos seguros
    chmod(ARCHIVO_TAREAS, 0600);
}

void agregar_tarea() {
    if (num_tareas >= MAX_TAREAS) {
        printf("No se pueden agregar más tareas. Límite alcanzado.\n");
        return;
    }

    Tarea nueva;
    printf("\nAgregar nueva tarea:\n");
    
    printf("Fecha (DD MM AAAA): ");
    if (scanf("%d %d %d", &nueva.dia, &nueva.mes, &nueva.anio) != 3) {
        printf("Entrada de fecha inválida\n");
        while (getchar() != '\n'); // Limpiar buffer
        return;
    }
    
    printf("Hora (HH MM): ");
    if (scanf("%d %d", &nueva.hora, &nueva.minuto) != 2) {
        printf("Entrada de hora inválida\n");
        while (getchar() != '\n');
        return;
    }
    
    printf("Nombre del script (solo nombre, sin ruta): ");
    char script_name[MAX_LONGITUD];
    getchar(); // Limpiar buffer
    if (fgets(script_name, MAX_LONGITUD, stdin) == NULL) {
        printf("Error al leer nombre del script\n");
        return;
    }
    script_name[strcspn(script_name, "\n")] = '\0';

    // Construir ruta segura
    snprintf(nueva.script, MAX_LONGITUD, "%s%s", ALLOWED_SCRIPT_DIR, script_name);
    
    if (!es_ruta_segura(nueva.script) || !archivo_es_seguro(nueva.script)) {
        printf("Error: Ruta de script no permitida\n");
        return;
    }

    nueva.ejecutada = 0;
    
    tareas[num_tareas++] = nueva;
    guardar_tareas();
    printf("Tarea agregada exitosamente.\n");
}

void ejecutar_script_seguro(const char *ruta) {
    char *args[] = {NULL};
    char *env[] = {NULL};

    pid_t pid = fork();
    if (pid == 0) {
        // Proceso hijo - ejecutar con permisos mínimos
        setgid(getgid());
        setuid(getuid());

        // Restringir entorno de ejecución
        execve(ruta, args, env);
        perror("Error al ejecutar el script");
        _exit(EXIT_FAILURE);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status)) {
            printf("Script ejecutado con código de salida: %d\n", WEXITSTATUS(status));
        }
    } else {
        perror("Error al crear proceso hijo");
    }
}

void verificar_y_ejecutar_tareas() {
    time_t ahora = time(NULL);
    struct tm tm_ahora;
    localtime_r(&ahora, &tm_ahora);
    
    for (int i = 0; i < num_tareas; i++) {
        if (tareas[i].ejecutada) continue;
        
        if (tareas[i].anio == tm_ahora.tm_year + 1900 &&
            tareas[i].mes == tm_ahora.tm_mon + 1 &&
            tareas[i].dia == tm_ahora.tm_mday &&
            tareas[i].hora == tm_ahora.tm_hour &&
            tareas[i].minuto == tm_ahora.tm_min) {
            
            printf("Ejecutando tarea programada: %s\n", tareas[i].script);
            
            if (archivo_es_seguro(tareas[i].script)) {
                ejecutar_script_seguro(tareas[i].script);
                tareas[i].ejecutada = 1;
                guardar_tareas();
            } else {
                fprintf(stderr, "Error: Script no es seguro para ejecución\n");
            }
        }
    }
}

// ... (resto de funciones como listar_tareas y menu permanecen igual)

int main() {
    // Ejecutar con privilegios mínimos
    if (getuid() == 0) {
        fprintf(stderr, "Error: No ejecutar como root\n");
        return EXIT_FAILURE;
    }

    cargar_tareas();
    verificar_y_ejecutar_tareas();
    menu();
    return EXIT_SUCCESS;
}