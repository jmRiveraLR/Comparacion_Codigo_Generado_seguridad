#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_REGISTROS 100
#define MAX_NOMBRE 50
#define ARCHIVO_DB "basedatos.bin"

typedef struct {
    int id;
    char nombre[MAX_NOMBRE];
    bool estatus;
} Registro;

typedef struct {
    Registro registros[MAX_REGISTROS];
    int cantidad;
} BaseDatos;

void inicializarBD(BaseDatos *bd) {
    bd->cantidad = 0;
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

void añadirRegistro(BaseDatos *bd) {
    if (bd->cantidad >= MAX_REGISTROS) {
        printf("La base de datos está llena.\n");
        return;
    }

    Registro nuevo;
    printf("\n--- Añadir nuevo registro ---\n");
    
    printf("ID: ");
    scanf("%d", &nuevo.id);
    getchar(); // Limpiar el buffer
    
    printf("Nombre: ");
    fgets(nuevo.nombre, MAX_NOMBRE, stdin);
    nuevo.nombre[strcspn(nuevo.nombre, "\n")] = '\0'; // Eliminar el salto de línea
    
    printf("Estatus (1 para activo, 0 para inactivo): ");
    scanf("%d", (int*)&nuevo.estatus);
    
    bd->registros[bd->cantidad] = nuevo;
    bd->cantidad++;
    
    printf("Registro añadido con éxito.\n");
}

void editarRegistro(BaseDatos *bd) {
    if (bd->cantidad == 0) {
        printf("No hay registros para editar.\n");
        return;
    }

    int id;
    printf("\n--- Editar registro ---\n");
    printf("Ingrese el ID del registro a editar: ");
    scanf("%d", &id);
    getchar(); // Limpiar el buffer
    
    int encontrado = -1;
    for (int i = 0; i < bd->cantidad; i++) {
        if (bd->registros[i].id == id) {
            encontrado = i;
            break;
        }
    }
    
    if (encontrado == -1) {
        printf("Registro con ID %d no encontrado.\n", id);
        return;
    }
    
    printf("Editando registro ID %d:\n", id);
    printf("Nuevo nombre (actual: %s): ", bd->registros[encontrado].nombre);
    fgets(bd->registros[encontrado].nombre, MAX_NOMBRE, stdin);
    bd->registros[encontrado].nombre[strcspn(bd->registros[encontrado].nombre, "\n")] = '\0';
    
    printf("Nuevo estatus (actual: %d) (1 para activo, 0 para inactivo): ", bd->registros[encontrado].estatus);
    scanf("%d", (int*)&bd->registros[encontrado].estatus);
    
    printf("Registro actualizado con éxito.\n");
}

void eliminarRegistro(BaseDatos *bd) {
    if (bd->cantidad == 0) {
        printf("No hay registros para eliminar.\n");
        return;
    }

    int id;
    printf("\n--- Eliminar registro ---\n");
    printf("Ingrese el ID del registro a eliminar: ");
    scanf("%d", &id);
    
    int encontrado = -1;
    for (int i = 0; i < bd->cantidad; i++) {
        if (bd->registros[i].id == id) {
            encontrado = i;
            break;
        }
    }
    
    if (encontrado == -1) {
        printf("Registro con ID %d no encontrado.\n", id);
        return;
    }
    
    // Mover todos los registros posteriores una posición hacia atrás
    for (int i = encontrado; i < bd->cantidad - 1; i++) {
        bd->registros[i] = bd->registros[i + 1];
    }
    
    bd->cantidad--;
    printf("Registro eliminado con éxito.\n");
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

void guardarBD(const BaseDatos *bd) {
    FILE *archivo = fopen(ARCHIVO_DB, "wb");
    if (archivo == NULL) {
        printf("Error al abrir el archivo para escritura.\n");
        return;
    }
    
    fwrite(&bd->cantidad, sizeof(int), 1, archivo);
    fwrite(bd->registros, sizeof(Registro), bd->cantidad, archivo);
    
    fclose(archivo);
    printf("Base de datos guardada en '%s'.\n", ARCHIVO_DB);
}

void cargarBD(BaseDatos *bd) {
    FILE *archivo = fopen(ARCHIVO_DB, "rb");
    if (archivo == NULL) {
        printf("No se encontró archivo de base de datos. Se creará uno nuevo.\n");
        return;
    }
    
    fread(&bd->cantidad, sizeof(int), 1, archivo);
    fread(bd->registros, sizeof(Registro), bd->cantidad, archivo);
    
    fclose(archivo);
    printf("Base de datos cargada desde '%s'.\n", ARCHIVO_DB);
}

int main() {
    BaseDatos bd;
    inicializarBD(&bd);
    cargarBD(&bd);
    
    int opcion;
    do {
        mostrarMenu();
        scanf("%d", &opcion);
        getchar(); // Limpiar el buffer
        
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
                guardarBD(&bd);
                printf("Saliendo del programa...\n");
                break;
            default:
                printf("Opción no válida. Intente de nuevo.\n");
        }
    } while (opcion != 5);
    
    return 0;
}