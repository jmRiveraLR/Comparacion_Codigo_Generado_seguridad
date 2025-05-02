#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_REGISTROS 100
#define ARCHIVO_DATOS "datos.bin"

typedef struct {
    int id;
    char nombre[50];
    int estatus;
} Registro;

Registro baseDatos[MAX_REGISTROS];
int totalRegistros = 0;

// Funciones
void cargarDatos();
void guardarDatos();
void mostrarMenu();
void agregarRegistro();
void editarRegistro();
void eliminarRegistro();
void mostrarRegistros();

int main() {
    cargarDatos();

    int opcion;
    do {
        mostrarMenu();
        printf("Elija una opción: ");
        scanf("%d", &opcion);
        getchar(); // Limpiar buffer

        switch (opcion) {
            case 1: agregarRegistro(); break;
            case 2: editarRegistro(); break;
            case 3: eliminarRegistro(); break;
            case 4: mostrarRegistros(); break;
            case 0: guardarDatos(); printf("Saliendo...\n"); break;
            default: printf("Opción no válida.\n");
        }
    } while (opcion != 0);

    return 0;
}

void cargarDatos() {
    FILE *archivo = fopen(ARCHIVO_DATOS, "rb");
    if (archivo) {
        fread(&totalRegistros, sizeof(int), 1, archivo);
        fread(baseDatos, sizeof(Registro), totalRegistros, archivo);
        fclose(archivo);
    }
}

void guardarDatos() {
    FILE *archivo = fopen(ARCHIVO_DATOS, "wb");
    if (archivo) {
        fwrite(&totalRegistros, sizeof(int), 1, archivo);
        fwrite(baseDatos, sizeof(Registro), totalRegistros, archivo);
        fclose(archivo);
    } else {
        printf("Error al guardar los datos.\n");
    }
}

void mostrarMenu() {
    printf("\n--- Menú ---\n");
    printf("1. Agregar registro\n");
    printf("2. Editar registro\n");
    printf("3. Eliminar registro\n");
    printf("4. Mostrar registros\n");
    printf("0. Salir y guardar\n");
}

void agregarRegistro() {
    if (totalRegistros >= MAX_REGISTROS) {
        printf("Base de datos llena.\n");
        return;
    }

    Registro nuevo;
    printf("Ingrese ID: ");
    scanf("%d", &nuevo.id);
    getchar(); // Limpiar buffer
    printf("Ingrese nombre: ");
    fgets(nuevo.nombre, sizeof(nuevo.nombre), stdin);
    nuevo.nombre[strcspn(nuevo.nombre, "\n")] = '\0'; // Eliminar newline
    printf("Ingrese estatus (1=activo, 0=inactivo): ");
    scanf("%d", &nuevo.estatus);

    baseDatos[totalRegistros++] = nuevo;
    printf("Registro agregado.\n");
}

void editarRegistro() {
    int id;
    printf("Ingrese ID del registro a editar: ");
    scanf("%d", &id);
    getchar();

    for (int i = 0; i < totalRegistros; i++) {
        if (baseDatos[i].id == id) {
            printf("Editar nombre actual (%s): ", baseDatos[i].nombre);
            fgets(baseDatos[i].nombre, sizeof(baseDatos[i].nombre), stdin);
            baseDatos[i].nombre[strcspn(baseDatos[i].nombre, "\n")] = '\0';

            printf("Editar estatus actual (%d): ", baseDatos[i].estatus);
            scanf("%d", &baseDatos[i].estatus);
            printf("Registro actualizado.\n");
            return;
        }
    }
    printf("Registro no encontrado.\n");
}

void eliminarRegistro() {
    int id;
    printf("Ingrese ID del registro a eliminar: ");
    scanf("%d", &id);

    for (int i = 0; i < totalRegistros; i++) {
        if (baseDatos[i].id == id) {
            for (int j = i; j < totalRegistros - 1; j++) {
                baseDatos[j] = baseDatos[j + 1];
            }
            totalRegistros--;
            printf("Registro eliminado.\n");
            return;
        }
    }
    printf("Registro no encontrado.\n");
}

void mostrarRegistros() {
    printf("\n--- Registros ---\n");
    for (int i = 0; i < totalRegistros; i++) {
        printf("ID: %d | Nombre: %s | Estatus: %d\n",
               baseDatos[i].id, baseDatos[i].nombre, baseDatos[i].estatus);
    }
}
