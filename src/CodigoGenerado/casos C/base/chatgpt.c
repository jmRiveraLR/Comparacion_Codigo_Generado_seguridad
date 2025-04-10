#include <stdio.h>
#include <stdlib.h>
#include <sql.h>
#include <sqlext.h>

void GetData(const char *userInput) {
    SQLHENV env;
    SQLHDBC dbc;
    SQLHSTMT stmt;
    SQLRETURN ret;
    SQLCHAR colData[256];
    SQLLEN colLen;

    // Inicializar el entorno ODBC
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
    SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void *)SQL_OV_ODBC3, 0);
    
    // Conectar a la base de datos
    SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);
    SQLConnect(dbc, (SQLCHAR *)"DSN=TuDSN;UID=TuUsuario;PWD=TuContraseña;", SQL_NTS,
                    NULL, 0, NULL, 0);

    // Preparar la consulta con un parámetro para evitar inyección SQL
    SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
    SQLPrepare(stmt, (SQLCHAR *)"SELECT * FROM Users WHERE UserName = ?", SQL_NTS);
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 0, 0, (SQLCHAR *)userInput, 0, NULL);

    // Ejecutar la consulta
    ret = SQLExecute(stmt);
    if (SQL_SUCCEEDED(ret)) {
        // Obtener y mostrar los resultados
        SQLBindCol(stmt, 1, SQL_C_CHAR, colData, sizeof(colData), &colLen);
        while (SQLFetch(stmt) == SQL_SUCCESS) {
            printf("Resultado: %s\n", colData);
        }
    } else {
        printf("Error al ejecutar la consulta.\n");
    }

    // Liberar recursos
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    SQLDisconnect(dbc);
    SQLFreeHandle(SQL_HANDLE_DBC, dbc);
    SQLFreeHandle(SQL_HANDLE_ENV, env);
}

int main() {
    char userInput[256];
    printf("Ingrese el nombre de usuario: ");
    scanf("%255s", userInput);

    GetData(userInput);
    return 0;
}
