#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int id;
    char nombre[50];
    char estatus[20];
} Registro;

typedef struct {
    Registro *registros;
    size_t size;
} BaseDatos;

void agregarRegistro(BaseDatos *db) {
    Registro nuevo;
    printf("Ingrese ID: ");
    scanf("%d", &nuevo.id);
    printf("Ingrese Nombre: ");
    scanf("%s", nuevo.nombre);
    printf("Ingrese Estatus: ");
    scanf("%s", nuevo.estatus);
    
    db->registros = realloc(db->registros, (db->size + 1) * sizeof(Registro));
    db->registros[db->size++] = nuevo;
}

void editarRegistro(BaseDatos *db) {
    int id;
    printf("Ingrese el ID del registro a editar: ");
    scanf("%d", &id);

    for (size_t i = 0; i < db->size; i++) {
        if (db->registros[i].id == id) {
            printf("Ingrese Nuevo Nombre: ");
            scanf("%s", db->registros[i].nombre);
            printf("Ingrese Nuevo Estatus: ");
            scanf("%s", db->registros[i].estatus);
            return;
        }
    }
    printf("Registro no encontrado.\n");
}

void eliminarRegistro(BaseDatos *db) {
    int id;
    printf("Ingrese el ID del registro a eliminar: ");
    scanf("%d", &id);

    for (size_t i = 0; i < db->size; i++) {
        if (db->registros[i].id == id) {
            for (size_t j = i; j < db->size - 1; j++) {
                db->registros[j] = db->registros[j + 1];
            }
            db->size--;
            db->registros = realloc(db->registros, db->size * sizeof(Registro));
            printf("Registro eliminado.\n");
            return;
        }
    }
    printf("Registro no encontrado.\n");
}

void guardarBaseDatos(BaseDatos *db, const char *archivo) {
    FILE *fp = fopen(archivo, "wb");
    if (fp) {
        fwrite(&db->size, sizeof(size_t), 1, fp);
        fwrite(db->registros, sizeof(Registro), db->size, fp);
        fclose(fp);
    } else {
        printf("Error al guardar los datos.\n");
    }
}

void cargarBaseDatos(BaseDatos *db, const char *archivo) {
    FILE *fp = fopen(archivo, "rb");
    if (fp) {
        fread(&db->size, sizeof(size_t), 1, fp);
        db->registros = malloc(db->size * sizeof(Registro));
        fread(db->registros, sizeof(Registro), db->size, fp);
        fclose(fp);
    }
}

void mostrarRegistros(BaseDatos *db) {
    for (size_t i = 0; i < db->size; i++) {
        printf("ID: %d, Nombre: %s, Estatus: %s\n", db->registros[i].id, db->registros[i].nombre, db->registros[i].estatus);
    }
}

int main() {
    BaseDatos db = {NULL, 0};
    const char *archivo = "basedatos.bin";

    cargarBaseDatos(&db, archivo);

    int opcion;
    do {
        printf("\nOpciones:\n");
        printf("1. Agregar Registro\n");
        printf("2. Editar Registro\n");
        printf("3. Eliminar Registro\n");
        printf("4. Mostrar Registros\n");
        printf("5. Salir\n");
        printf("Seleccione una opcion: ");
        scanf("%d", &opcion);

        switch (opcion) {
            case 1: agregarRegistro(&db); break;
            case 2: editarRegistro(&db); break;
            case 3: eliminarRegistro(&db); break;
            case 4: mostrarRegistros(&db); break;
            case 5: guardarBaseDatos(&db, archivo); break;
            default: printf("Opcion invalida.\n");
        }
    } while (opcion != 5);

    free(db.registros);
    return 0;
}