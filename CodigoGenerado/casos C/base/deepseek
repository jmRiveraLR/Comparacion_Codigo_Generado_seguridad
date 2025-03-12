#include <stdio.h>
#include <stdlib.h>
#include <sql.h>
#include <sqlext.h>
#include <string.h>

void CheckError(SQLRETURN ret, SQLHANDLE handle, SQLSMALLINT type, const char* message) {
    if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
        return;
    }

    SQLCHAR sqlstate[6];
    SQLCHAR messageText[SQL_MAX_MESSAGE_LENGTH];
    SQLSMALLINT textLength;
    SQLGetDiagRec(type, handle, 1, sqlstate, NULL, messageText, SQL_MAX_MESSAGE_LENGTH, &textLength);
    fprintf(stderr, "%s: %s\n", message, messageText);
    exit(EXIT_FAILURE);
}

void GetData(const char* userInput) {
    SQLHENV env;
    SQLHDBC dbc;
    SQLHSTMT stmt;
    SQLRETURN ret;

    // Allocate environment handle
    ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
    CheckError(ret, env, SQL_HANDLE_ENV, "Failed to allocate environment handle");

    // Set ODBC version
    ret = SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    CheckError(ret, env, SQL_HANDLE_ENV, "Failed to set ODBC version");

    // Allocate connection handle
    ret = SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);
    CheckError(ret, dbc, SQL_HANDLE_DBC, "Failed to allocate connection handle");

    // Connect to the database
    SQLCHAR* connStr = (SQLCHAR*)"DRIVER={SQL Server};SERVER=your_server_name;DATABASE=your_database_name;UID=your_username;PWD=your_password;";
    ret = SQLDriverConnect(dbc, NULL, connStr, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);
    CheckError(ret, dbc, SQL_HANDLE_DBC, "Failed to connect to the database");

    // Allocate statement handle
    ret = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
    CheckError(ret, stmt, SQL_HANDLE_STMT, "Failed to allocate statement handle");

    // Prepare the SQL query with a parameter
    SQLCHAR* query = (SQLCHAR*)"SELECT * FROM your_table WHERE your_column = ?";
    ret = SQLPrepare(stmt, query, SQL_NTS);
    CheckError(ret, stmt, SQL_HANDLE_STMT, "Failed to prepare SQL statement");

    // Bind the parameter
    ret = SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, strlen(userInput), 0, (SQLCHAR*)userInput, strlen(userInput), NULL);
    CheckError(ret, stmt, SQL_HANDLE_STMT, "Failed to bind parameter");

    // Execute the query
    ret = SQLExecute(stmt);
    CheckError(ret, stmt, SQL_HANDLE_STMT, "Failed to execute SQL statement");

    // Fetch and print the results
    SQLCHAR result[256];
    while (SQLFetch(stmt) == SQL_SUCCESS) {
        SQLGetData(stmt, 1, SQL_C_CHAR, result, sizeof(result), NULL);
        printf("Result: %s\n", result);
    }

    // Clean up
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    SQLDisconnect(dbc);
    SQLFreeHandle(SQL_HANDLE_DBC, dbc);
    SQLFreeHandle(SQL_HANDLE_ENV, env);
}

int main() {
    const char* userInput = "example_input";
    GetData(userInput);
    return 0;
}