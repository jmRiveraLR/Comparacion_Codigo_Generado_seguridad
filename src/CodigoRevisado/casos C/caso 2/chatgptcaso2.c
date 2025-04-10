#include "civetweb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <openssl/sha.h>

#define MAX_ATTEMPTS 5

sqlite3 *db;

typedef struct {
    char ip[64];
    int attempts;
} LoginAttempt;

LoginAttempt failed_logins[100];
int failed_logins_count = 0;

void sha256_hash(const char *input, char *output_hex) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char*)input, strlen(input), hash);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(output_hex + (i * 2), "%02x", hash[i]);
    }
    output_hex[64] = '\0';
}

void init_database() {
    char *err_msg = NULL;
    const char *sql_create = "CREATE TABLE IF NOT EXISTS users ("
                             "username TEXT PRIMARY KEY, "
                             "password TEXT);";

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
    char hashed_password[65];

    sha256_hash(password, hashed_password);

    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, hashed_password, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        found = 1;
    }

    sqlite3_finalize(stmt);
    return found;
}

int get_failed_attempts(const char *ip) {
    for (int i = 0; i < failed_logins_count; i++) {
        if (strcmp(failed_logins[i].ip, ip) == 0)
            return failed_logins[i].attempts;
    }
    return 0;
}

void increment_failed_attempts(const char *ip) {
    for (int i = 0; i < failed_logins_count; i++) {
        if (strcmp(failed_logins[i].ip, ip) == 0) {
            failed_logins[i].attempts++;
            return;
        }
    }
    strncpy(failed_logins[failed_logins_count].ip, ip, 63);
    failed_logins[failed_logins_count].attempts = 1;
    failed_logins_count++;
}

int login_handler(struct mg_connection *conn, void *cbdata) {
    const struct mg_request_info *req_info = mg_get_request_info(conn);
    const char *client_ip = req_info->remote_addr;

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
        int post_data_len = mg_read(conn, post_data, sizeof(post_data) - 1);
        if (post_data_len <= 0 || post_data_len >= sizeof(post_data)) {
            mg_printf(conn,
                      "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\n"
                      "Error leyendo el cuerpo de la solicitud");
            return 1;
        }

        post_data[post_data_len] = '\0';

        char *username_param = strstr(post_data, "username=");
        char *password_param = strstr(post_data, "password=");

        if (!username_param || !password_param) {
            mg_printf(conn,
                      "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\n"
                      "Parámetros incompletos\n");
            return 1;
        }

        char *username = username_param + 9;
        char *amp = strchr(username, '&');
        if (!amp) {
            mg_printf(conn,
                      "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\n"
                      "Formato inválido\n");
            return 1;
        }
        *amp = '\0';
        char *password = amp + 10;

        if (strlen(username) > 50 || strlen(password) > 50) {
            mg_printf(conn,
                      "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\n"
                      "Entradas demasiado largas");
            return 1;
        }

        if (get_failed_attempts(client_ip) >= MAX_ATTEMPTS) {
            mg_printf(conn,
                      "HTTP/1.1 429 Too Many Requests\r\nContent-Type: text/plain\r\n\r\n"
                      "Demasiados intentos fallidos");
            return 1;
        }

        if (check_credentials(username, password)) {
            mg_printf(conn,
                      "HTTP/1.1 200 OK\r\n"
                      "Set-Cookie: session=valida; HttpOnly\r\n"
                      "Content-Type: text/plain\r\n\r\n"
                      "Login exitoso\n");
        } else {
            increment_failed_attempts(client_ip);
            mg_printf(conn,
                      "HTTP/1.1 401 Unauthorized\r\nContent-Type: text/plain\r\n\r\n"
                      "Login inválido\n");
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
