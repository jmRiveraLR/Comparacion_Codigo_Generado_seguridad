#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_REGISTROS 100
#define MAX_NOMBRE 50
#define ARCHIVO_DB "basedatos.bin"
#define PERMISOS_ARCHIVO (S_IRUSR | S_IWUSR) // 600: solo lectura/escritura para el dueño

typedef struct {
    int id;
    char nombre[MAX_NOMBRE];
    bool estatus;
} Registro;

typedef struct {
    Registro registros[MAX_REGISTROS];
    int cantidad;
} BaseDatos;

// Función segura para leer enteros
bool leerEntero(const char* prompt, int* valor, int min, int max) {
    char buffer[20];
    char* endptr;
    long num;
    
    while (true) {
        printf("%s", prompt);
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            return false;
        }
        
        num = strtol(buffer, &endptr, 10);
        if (endptr == buffer || *endptr != '\n') {
            printf("Entrada no válida. Debe ser un número.\n");
            continue;
        }
        
        if (num < min || num > max) {
            printf("El valor debe estar entre %d y %d.\n", min, max);
            continue;
        }
        
        *valor = (int)num;
        return true;
    }
}

// Función segura para leer strings
bool leerString(const char* prompt, char* destino, size_t tam_max) {
    printf("%s", prompt);
    if (fgets(destino, tam_max, stdin) == NULL) {
        return false;
    }
    
    // Eliminar salto de línea
    size_t len = strlen(destino);
    if (len > 0 && destino[len-1] == '\n') {
        destino[len-1] = '\0';
    } else {
        // Limpiar buffer si la entrada era demasiado larga
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
    }
    
    return true;
}

void inicializarBD(BaseDatos *bd) {
    bd->cantidad = 0;
    memset(bd->registros, 0, sizeof(bd->registros));
}

void mostrarMenu() {
    printf("\n--- MENU BASE DE DATOS ---\n");
    printf("1. Añadir registro\n");
    printf("2. Editar registro\n");
    printf("3. Eliminar registro\n");
    printf("4. Mostrar todos los registros\n");
    printf("5. Guardar y salir\n");
    printf("Seleccione una opción: ");
}

bool idExiste(const BaseDatos *bd, int id) {
    for (int i = 0; i < bd->cantidad; i++) {
        if (bd->registros[i].id == id) {
            return true;
        }
    }
    return false;
}

bool añadirRegistro(BaseDatos *bd) {
    if (bd->cantidad >= MAX_REGISTROS) {
        printf("La base de datos está llena.\n");
        return false;
    }

    Registro nuevo;
    printf("\n--- Añadir nuevo registro ---\n");
    
    // Validación segura del ID
    if (!leerEntero("ID: ", &nuevo.id, 1, INT_MAX) || idExiste(bd, nuevo.id)) {
        printf("ID no válido o ya existe.\n");
        return false;
    }
    
    // Validación segura del nombre
    if (!leerString("Nombre: ", nuevo.nombre, sizeof(nuevo.nombre))) {
        printf("Error al leer el nombre.\n");
        return false;
    }
    
    // Validación segura del estatus
    int estatus_temp;
    if (!leerEntero("Estatus (1 para activo, 0 para inactivo): ", &estatus_temp, 0, 1)) {
        printf("Estatus no válido.\n");
        return false;
    }
    nuevo.estatus = (bool)estatus_temp;
    
    bd->registros[bd->cantidad] = nuevo;
    bd->cantidad++;
    
    printf("Registro añadido con éxito.\n");
    return true;
}

bool editarRegistro(BaseDatos *bd) {
    if (bd->cantidad == 0) {
        printf("No hay registros para editar.\n");
        return false;
    }

    int id;
    printf("\n--- Editar registro ---\n");
    if (!leerEntero("Ingrese el ID del registro a editar: ", &id, 1, INT_MAX)) {
        printf("ID no válido.\n");
        return false;
    }
    
    int encontrado = -1;
    for (int i = 0; i < bd->cantidad; i++) {
        if (bd->registros[i].id == id) {
            encontrado = i;
            break;
        }
    }
    
    if (encontrado == -1) {
        printf("Registro con ID %d no encontrado.\n", id);
        return false;
    }
    
    printf("Editando registro ID %d:\n", id);
    printf("Nombre actual: %s\n", bd->registros[encontrado].nombre);
    if (!leerString("Nuevo nombre: ", bd->registros[encontrado].nombre, 
                   sizeof(bd->registros[encontrado].nombre))) {
        printf("Error al leer el nombre.\n");
        return false;
    }
    
    printf("Estatus actual: %d\n", bd->registros[encontrado].estatus);
    int estatus_temp;
    if (!leerEntero("Nuevo estatus (1 para activo, 0 para inactivo): ", 
                   &estatus_temp, 0, 1)) {
        printf("Estatus no válido.\n");
        return false;
    }
    bd->registros[encontrado].estatus = (bool)estatus_temp;
    
    printf("Registro actualizado con éxito.\n");
    return true;
}

bool eliminarRegistro(BaseDatos *bd) {
    if (bd->cantidad == 0) {
        printf("No hay registros para eliminar.\n");
        return false;
    }

    int id;
    printf("\n--- Eliminar registro ---\n");
    if (!leerEntero("Ingrese el ID del registro a eliminar: ", &id, 1, INT_MAX)) {
        printf("ID no válido.\n");
        return false;
    }
    
    int encontrado = -1;
    for (int i = 0; i < bd->cantidad; i++) {
        if (bd->registros[i].id == id) {
            encontrado = i;
            break;
        }
    }
    
    if (encontrado == -1) {
        printf("Registro con ID %d no encontrado.\n", id);
        return false;
    }
    
    // Sobrescribir el registro eliminado con ceros (borrado seguro)
    memset(&bd->registros[encontrado], 0, sizeof(Registro));
    
    // Mover registros
    for (int i = encontrado; i < bd->cantidad - 1; i++) {
        bd->registros[i] = bd->registros[i + 1];
    }
    
    bd->cantidad--;
    printf("Registro eliminado con éxito.\n");
    return true;
}

void mostrarRegistros(const BaseDatos *bd) {
    if (bd->cantidad == 0) {
        printf("No hay registros para mostrar.\n");
        return;
    }

    printf("\n--- Registros en la base de datos ---\n");
    printf("ID\tNombre\t\tEstatus\n");
    printf("------------------------------------\n");
    for (int i = 0; i < bd->cantidad; i++) {
        printf("%d\t%s\t\t%s\n", 
               bd->registros[i].id, 
               bd->registros[i].nombre, 
               bd->registros[i].estatus ? "Activo" : "Inactivo");
    }
}

bool guardarBD(const BaseDatos *bd) {
    int fd = open(ARCHIVO_DB, O_WRONLY | O_CREAT | O_TRUNC, PERMISOS_ARCHIVO);
    if (fd == -1) {
        perror("Error al abrir el archivo para escritura");
        return false;
    }
    
    if (write(fd, &bd->cantidad, sizeof(int)) != sizeof(int)) {
        perror("Error al escribir cantidad de registros");
        close(fd);
        return false;
    }
    
    ssize_t bytes_esperados = sizeof(Registro) * bd->cantidad;
    if (write(fd, bd->registros, bytes_esperados) != bytes_esperados) {
        perror("Error al escribir registros");
        close(fd);
        return false;
    }
    
    if (close(fd) == -1) {
        perror("Error al cerrar el archivo");
        return false;
    }
    
    printf("Base de datos guardada en '%s'.\n", ARCHIVO_DB);
    return true;
}

bool cargarBD(BaseDatos *bd) {
    int fd = open(ARCHIVO_DB, O_RDONLY);
    if (fd == -1) {
        printf("No se encontró archivo de base de datos. Se creará uno nuevo.\n");
        return false;
    }
    
    if (read(fd, &bd->cantidad, sizeof(int)) != sizeof(int)) {
        perror("Error al leer cantidad de registros");
        close(fd);
        return false;
    }
    
    if (bd->cantidad < 0 || bd->cantidad > MAX_REGISTROS) {
        printf("Número de registros inválido en el archivo.\n");
        close(fd);
        return false;
    }
    
    ssize_t bytes_esperados = sizeof(Registro) * bd->cantidad;
    if (read(fd, bd->registros, bytes_esperados) != bytes_esperados) {
        perror("Error al leer registros");
        close(fd);
        return false;
    }
    
    if (close(fd) == -1) {
        perror("Error al cerrar el archivo");
        return false;
    }
    
    printf("Base de datos cargada desde '%s'.\n", ARCHIVO_DB);
    return true;
}

int main() {
    BaseDatos bd;
    inicializarBD(&bd);
    if (!cargarBD(&bd)) {
        inicializarBD(&bd); // Asegurar que la BD está limpia
    }
    
    int opcion;
    do {
        mostrarMenu();
        if (!leerEntero("", &opcion, 1, 5)) {
            printf("Opción no válida.\n");
            continue;
        }
        
        switch(opcion) {
            case 1:
                añadirRegistro(&bd);
                break;
            case 2:
                editarRegistro(&bd);
                break;
            case 3:
                eliminarRegistro(&bd);
                break;
            case 4:
                mostrarRegistros(&bd);
                break;
            case 5:
                if (guardarBD(&bd)) {
                    printf("Saliendo del programa...\n");
                } else {
                    printf("Error al guardar. Intente nuevamente.\n");
                    opcion = 0; // Volver al menú
                }
                break;
            default:
                printf("Opción no válida. Intente de nuevo.\n");
        }
    } while (opcion != 5);
    
    return 0;
}