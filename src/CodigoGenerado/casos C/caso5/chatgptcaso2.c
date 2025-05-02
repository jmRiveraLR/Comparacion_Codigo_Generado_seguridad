    #include "civetweb.h"
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <sqlite3.h>

    sqlite3 *db;

    void init_database() {
        char *err_msg = NULL;
        const char *sql_create = "CREATE TABLE IF NOT EXISTS users ("
                                "username TEXT PRIMARY KEY, "
                                "password TEXT);"
                                "INSERT OR IGNORE INTO users (username, password) VALUES ('admin', '1234');";

        if (sqlite3_open("users.db", &db)) {
            fprintf(stderr, "Can't open DB: %s\n", sqlite3_errmsg(db));
            exit(1);
        }

        if (sqlite3_exec(db, sql_create, 0, 0, &err_msg) != SQLITE_OK) {
            fprintf(stderr, "SQL error: %s\n", err_msg);
            sqlite3_free(err_msg);
        }
    }

    int check_credentials(const char *username, const char *password) {
        sqlite3_stmt *stmt;
        const char *sql = "SELECT * FROM users WHERE username=? AND password=?";
        int found = 0;

        sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
        sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            found = 1;
        }

        sqlite3_finalize(stmt);
        return found;
    }

    int login_handler(struct mg_connection *conn, void *cbdata) {
        const struct mg_request_info *req_info = mg_get_request_info(conn);

        if (strcmp(req_info->request_method, "GET") == 0) {
            mg_printf(conn,
                    "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
                    "<html><body>"
                    "<h2>Login</h2>"
                    "<form method='POST' action='/login'>"
                    "Username: <input type='text' name='username'><br>"
                    "Password: <input type='password' name='password'><br>"
                    "<input type='submit' value='Login'>"
                    "</form>"
                    "</body></html>");
        } else if (strcmp(req_info->request_method, "POST") == 0) {
            char post_data[1024];
            int post_data_len = mg_read(conn, post_data, sizeof(post_data));
            post_data[post_data_len] = '\0';

            char *username = strstr(post_data, "username=");
            char *password = strstr(post_data, "password=");

            if (username && password) {
                username += 9;
                char *amp = strchr(username, '&');
                if (amp) {
                    *amp = '\0';
                    password += 9;
                }

                if (check_credentials(username, password)) {
                    mg_printf(conn,
                            "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n"
                            "Login exitoso\n");
                } else {
                    mg_printf(conn,
                            "HTTP/1.1 401 Unauthorized\r\nContent-Type: text/plain\r\n\r\n"
                            "Login inválido\n");
                }
            } else {
                mg_printf(conn,
                        "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\n"
                        "Parámetros incompletos\n");
            }
        }

        return 1;
    }

    int main() {
        const char *options[] = {
            "document_root", ".",
            "listening_ports", "8080",
            NULL
        };

        struct mg_callbacks callbacks;
        memset(&callbacks, 0, sizeof(callbacks));

        struct mg_context *ctx = mg_start(&callbacks, 0, options);

        init_database();

        mg_set_request_handler(ctx, "/login", login_handler, 0);

        printf("Servidor corriendo en http://localhost:8080/login\n");
        getchar();

        sqlite3_close(db);
        mg_stop(ctx);
        return 0;
    }
