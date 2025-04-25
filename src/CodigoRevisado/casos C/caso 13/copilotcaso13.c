#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MAX_NOMBRE 50
#define MAX_ESTATUS 20

typedef struct {
    int id;
    char nombre[MAX_NOMBRE];
    char estatus[MAX_ESTATUS];
} Registro;

typedef struct {
    Registro *registros;
    size_t size;
} BaseDatos;

void agregarRegistro(BaseDatos *db) {
    Registro nuevo;
    printf("Ingrese ID: ");
    if (scanf("%d", &nuevo.id) != 1) {
        printf("Entrada inválida.\n");
        return;
    }
    getchar();  // Consumir el salto de línea restante.
    
    printf("Ingrese Nombre (máximo %d caracteres): ", MAX_NOMBRE - 1);
    if (!fgets(nuevo.nombre, MAX_NOMBRE, stdin)) {
        printf("Error al leer el nombre.\n");
        return;
    }
    nuevo.nombre[strcspn(nuevo.nombre, "\n")] = '\0';  // Eliminar el salto de línea.

    printf("Ingrese Estatus (máximo %d caracteres): ", MAX_ESTATUS - 1);
    if (!fgets(nuevo.estatus, MAX_ESTATUS, stdin)) {
        printf("Error al leer el estatus.\n");
        return;
    }
    nuevo.estatus[strcspn(nuevo.estatus, "\n")] = '\0';  // Eliminar el salto de línea.
    
    Registro *temp = realloc(db->registros, (db->size + 1) * sizeof(Registro));
    if (temp == NULL) {
        printf("Error al asignar memoria.\n");
        return;
    }
    db->registros = temp;
    db->registros[db->size++] = nuevo;
}

void editarRegistro(BaseDatos *db) {
    int id;
    printf("Ingrese el ID del registro a editar: ");
    if (scanf("%d", &id) != 1) {
        printf("Entrada inválida.\n");
        return;
    }
    getchar();  // Consumir el salto de línea restante.

    for (size_t i = 0; i < db->size; i++) {
        if (db->registros[i].id == id) {
            printf("Ingrese Nuevo Nombre (máximo %d caracteres): ", MAX_NOMBRE - 1);
            if (!fgets(db->registros[i].nombre, MAX_NOMBRE, stdin)) {
                printf("Error al leer el nombre.\n");
                return;
            }
            db->registros[i].nombre[strcspn(db->registros[i].nombre, "\n")] = '\0';

            printf("Ingrese Nuevo Estatus (máximo %d caracteres): ", MAX_ESTATUS - 1);
            if (!fgets(db->registros[i].estatus, MAX_ESTATUS, stdin)) {
                printf("Error al leer el estatus.\n");
                return;
            }
            db->registros[i].estatus[strcspn(db->registros[i].estatus, "\n")] = '\0';
            return;
        }
    }
    printf("Registro no encontrado.\n");
}

void eliminarRegistro(BaseDatos *db) {
    int id;
    printf("Ingrese el ID del registro a eliminar: ");
    if (scanf("%d", &id) != 1) {
        printf("Entrada inválida.\n");
        return;
    }

    for (size_t i = 0; i < db->size; i++) {
        if (db->registros[i].id == id) {
            for (size_t j = i; j < db->size - 1; j++) {
                db->registros[j] = db->registros[j + 1];
            }
            db->size--;
            Registro *temp = realloc(db->registros, db->size * sizeof(Registro));
            if (temp != NULL || db->size == 0) {
                db->registros = temp;
            }
            printf("Registro eliminado.\n");
            return;
        }
    }
    printf("Registro no encontrado.\n");
}

void guardarBaseDatos(BaseDatos *db, const char *archivo) {
    FILE *fp = fopen(archivo, "wb");
    if (fp) {
        if (fwrite(&db->size, sizeof(size_t), 1, fp) != 1 || 
            fwrite(db->registros, sizeof(Registro), db->size, fp) != db->size) {
            printf("Error al guardar los datos.\n");
        }
        fclose(fp);
    } else {
        printf("Error al abrir el archivo: %s\n", strerror(errno));
    }
}

void cargarBaseDatos(BaseDatos *db, const char *archivo) {
    FILE *fp = fopen(archivo, "rb");
    if (fp) {
        if (fread(&db->size, sizeof(size_t), 1, fp) != 1) {
            printf("Error al cargar el tamaño de los datos.\n");
            fclose(fp);
            return;
        }
        db->registros = malloc(db->size * sizeof(Registro));
        if (db->registros == NULL) {
            printf("Error al asignar memoria.\n");
            fclose(fp);
            return;
        }
        if (fread(db->registros, sizeof(Registro), db->size, fp) != db->size) {
            printf("Error al cargar los registros.\n");
            free(db->registros);
            db->registros = NULL;
            db->size = 0;
        }
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
        if (scanf("%d", &opcion) != 1) {
            printf("Entrada inválida.\n");
            opcion = 0;  // Reiniciar para evitar salir prematuramente.
        }

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