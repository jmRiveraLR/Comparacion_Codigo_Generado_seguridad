#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sql.h>
#include <sqlext.h>
#include <bcrypt.h> // Biblioteca de bcrypt (debe instalarse)

#define MAX_USERNAME 256
#define MAX_PASSWORD 256
#define HASH_SIZE 60 // Tamaño del hash bcrypt

// Variables globales para la conexión ODBC
SQLHENV hEnv;
SQLHDBC hDbc;

// Función para inicializar la conexión a SQL Server
int conectar_bd() {
    if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv) != SQL_SUCCESS) return 0;
    if (SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0) != SQL_SUCCESS) return 0;
    if (SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc) != SQL_SUCCESS) return 0;

    // Reemplazar con cadena de conexión segura (no usar credenciales en código)
    SQLCHAR connectionString[] = "DSN=TuDSN;UID=UsuarioSeguro;PWD=ContraseñaSegura";
    SQLRETURN ret = SQLDriverConnect(hDbc, NULL, connectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);

    return (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO);
}

// Función para registrar un usuario con contraseña cifrada
void registrar_usuario(const char *username, const char *password) {
    if (strlen(username) == 0 || strlen(password) == 0) {
        printf("Error: Usuario y contraseña no pueden estar vacíos.\n");
        return;
    }

    char hash[HASH_SIZE];
    if (bcrypt_hashpw(password, hash) != 0) { // Cifra la contraseña
        printf("Error al generar el hash.\n");
        return;
    }

    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    SQLPrepare(hStmt, (SQLCHAR *)"INSERT INTO Users (UserName, PasswordHash) VALUES (?, ?)", SQL_NTS);
    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 0, 0, (SQLPOINTER)username, 0, NULL);
    SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 0, 0, (SQLPOINTER)hash, 0, NULL);

    if (SQLExecute(hStmt) == SQL_SUCCESS) {
        printf("Usuario '%s' registrado exitosamente.\n", username);
    } else {
        printf("Error: No se pudo registrar al usuario.\n");
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
}

// Función para iniciar sesión
void iniciar_sesion(const char *username, const char *password) {
    SQLHSTMT hStmt;
    SQLCHAR stored_hash[HASH_SIZE];

    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    SQLPrepare(hStmt, (SQLCHAR *)"SELECT PasswordHash FROM Users WHERE UserName = ?", SQL_NTS);
    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 0, 0, (SQLPOINTER)username, 0, NULL);
    SQLExecute(hStmt);

    if (SQLFetch(hStmt) == SQL_SUCCESS) {
        SQLGetData(hStmt, 1, SQL_C_CHAR, stored_hash, HASH_SIZE, NULL);
        if (bcrypt_checkpw(password, (const char *)stored_hash) == 0) {
            printf("Inicio de sesión exitoso. Bienvenido, %s.\n", username);
        } else {
            printf("Contraseña incorrecta.\n");
        }
    } else {
        printf("Usuario no encontrado.\n");
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
}

// Función principal con menú
int main() {
    if (!conectar_bd()) {
        printf("Error: No se pudo conectar a la base de datos.\n");
        return 1;
    }

    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    int opcion;

    while (1) {
        printf("\n--- Menú ---\n");
        printf("1. Iniciar sesión\n");
        printf("2. Registrar usuario\n");
        printf("3. Salir\n");
        printf("Seleccione una opción: ");
        scanf("%d", &opcion);
        getchar(); // Limpiar buffer

        if (opcion == 1) {
            printf("Nombre de usuario: ");
            fgets(username, MAX_USERNAME, stdin);
            username[strcspn(username, "\n")] = 0; // Eliminar salto de línea

            printf("Contraseña: ");
            fgets(password, MAX_PASSWORD, stdin);
            password[strcspn(password, "\n")] = 0;

            iniciar_sesion(username, password);
        } else if (opcion == 2) {
            printf("Nombre de usuario: ");
            fgets(username, MAX_USERNAME, stdin);
            username[strcspn(username, "\n")] = 0;

            printf("Contraseña: ");
            fgets(password, MAX_PASSWORD, stdin);
            password[strcspn(password, "\n")] = 0;

            registrar_usuario(username, password);
        } else if (opcion == 3) {
            printf("Saliendo...\n");
            break;
        } else {
            printf("Opción no válida.\n");
        }
    }

    // Liberar recursos
    SQLDisconnect(hDbc);
    SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
    SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
    return 0;
}
