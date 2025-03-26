#include <stdio.h>
#include <stdlib.h>
#include <sql.h>
#include <sqlext.h>

void GetData(char* userInput);

int main() {
    char userInput[256];
    printf("Ingrese el parámetro de consulta: ");
    scanf("%255s", userInput);
    GetData(userInput);
    return 0;
}

void GetData(char* userInput) {
    SQLHENV hEnv = NULL;
    SQLHDBC hDbc = NULL;
    SQLHSTMT hStmt = NULL;
    SQLRETURN ret;
    char connStr[] = "DRIVER={SQL Server};SERVER=your_server_name;DATABASE=your_database_name;UID=your_username;PWD=your_password;";

    // Asignar un entorno ODBC
    ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        fprintf(stderr, "Error al asignar el entorno ODBC.\n");
        return;
    }

    // Establecer la versión ODBC
    ret = SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        fprintf(stderr, "Error al establecer la versión ODBC.\n");
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
        return;
    }

    // Asignar una conexión ODBC
    ret = SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        fprintf(stderr, "Error al asignar la conexión ODBC.\n");
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
        return;
    }

    // Conectar a la base de datos
    ret = SQLDriverConnect(hDbc, NULL, (SQLCHAR*)connStr, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        fprintf(stderr, "Error al conectar a la base de datos.\n");
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
        return;
    }

    // Asignar un handle de declaración
    ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        fprintf(stderr, "Error al asignar el handle de declaración.\n");
        SQLDisconnect(hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
        return;
    }

    // Preparar y ejecutar la consulta SQL parametrizada
    char sqlQuery[] = "SELECT * FROM your_table_name WHERE column_name = ?";
    ret = SQLPrepare(hStmt, (SQLCHAR*)sqlQuery, SQL_NTS);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        fprintf(stderr, "Error al preparar la consulta SQL.\n");
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        SQLDisconnect(hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
        return;
    }

    // Vincular el parámetro de entrada
    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, userInput, 0, NULL);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        fprintf(stderr, "Error al vincular el parámetro de entrada.\n");
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        SQLDisconnect(hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
        return;
    }

    // Ejecutar la consulta SQL
    ret = SQLExecute(hStmt);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        fprintf(stderr, "Error al ejecutar la consulta SQL.\n");
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        SQLDisconnect(hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
        return;
    }

    // Procesar los resultados
    SQLCHAR columnData[256];
    while (SQLFetch(hStmt) == SQL_SUCCESS) {
        SQLGetData(hStmt, 1, SQL_C_CHAR, columnData, sizeof(columnData), NULL);
        printf("Resultado: %s\n", columnData);
    }

    // Liberar recursos
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    SQLDisconnect(hDbc);
    SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
    SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
}
