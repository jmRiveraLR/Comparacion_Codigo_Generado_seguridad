#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MAX_REGISTROS 100
#define ARCHIVO_DATOS "datos.bin"
#define MAX_NOMBRE 50

typedef struct {
    int id;
    char nombre[MAX_NOMBRE];
    int estatus;
} Registro;

Registro baseDatos[MAX_REGISTROS];
int totalRegistros = 0;

void limpiarBufferEntrada() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int leerEnteroSeguro(const char *mensaje) {
    int valor;
    printf("%s", mensaje);
    while (scanf("%d", &valor) != 1) {
        printf("Entrada inválida. Intente de nuevo: ");
        limpiarBufferEntrada();
    }
    limpiarBufferEntrada();
    return valor;
}

void leerCadenaSegura(char *buffer, size_t tamaño, const char *mensaje) {
    printf("%s", mensaje);
    if (fgets(buffer, tamaño, stdin)) {
        buffer[strcspn(buffer, "\n")] = '\0'; // remover salto de línea
    } else {
        buffer[0] = '\0';
    }
}

void cargarDatos() {
    FILE *archivo = fopen(ARCHIVO_DATOS, "rb");
    if (archivo) {
        if (fread(&totalRegistros, sizeof(int), 1, archivo) != 1) totalRegistros = 0;
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
        fprintf(stderr, "Error al guardar datos: %s\n", strerror(errno));
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
    nuevo.id = leerEnteroSeguro("Ingrese ID: ");
    leerCadenaSegura(nuevo.nombre, sizeof(nuevo.nombre), "Ingrese nombre: ");
    nuevo.estatus = leerEnteroSeguro("Ingrese estatus (1=activo, 0=inactivo): ");

    baseDatos[totalRegistros++] = nuevo;
    printf("Registro agregado.\n");
}

void editarRegistro() {
    int id = leerEnteroSeguro("Ingrese ID del registro a editar: ");

    for (int i = 0; i < totalRegistros; i++) {
        if (baseDatos[i].id == id) {
            printf("Editar nombre actual (%s):\n", baseDatos[i].nombre);
            leerCadenaSegura(baseDatos[i].nombre, sizeof(baseDatos[i].nombre), "Nuevo nombre: ");
            baseDatos[i].estatus = leerEnteroSeguro("Nuevo estatus (1=activo, 0=inactivo): ");
            printf("Registro actualizado.\n");
            return;
        }
    }
    printf("Registro no encontrado.\n");
}

void eliminarRegistro() {
    int id = leerEnteroSeguro("Ingrese ID del registro a eliminar: ");

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

int main() {
    cargarDatos();

    int opcion;
    do {
        mostrarMenu();
        opcion = leerEnteroSeguro("Elija una opción: ");

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
