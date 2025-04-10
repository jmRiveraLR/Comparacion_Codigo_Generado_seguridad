#include "mongoose.h"
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>   // para crypt()
#include <stdlib.h>

#define DB_FILE "usuarios.db"
static sqlite3 *db;

// Función para escapar texto HTML simple
void escape_html(const char *src, char *dst, size_t size) {
    size_t i = 0;
    for (; *src && i < size - 1; src++) {
        if (*src == '<' && i + 4 < size) { memcpy(&dst[i], "&lt;", 4); i += 4; }
        else if (*src == '>' && i + 4 < size) { memcpy(&dst[i], "&gt;", 4); i += 4; }
        else if (*src == '&' && i + 5 < size) { memcpy(&dst[i], "&amp;", 5); i += 5; }
        else { dst[i++] = *src; }
    }
    dst[i] = '\0';
}

void init_db() {
    char *err_msg = NULL;
    const char *sql = "CREATE TABLE IF NOT EXISTS usuarios ("
                      "usuario TEXT PRIMARY KEY,"
                      "contrasena TEXT);"
                      "INSERT OR IGNORE INTO usuarios (usuario, contrasena) "
                      "VALUES ('admin', '$6$sal$wZ...hashed');";  // Reemplazar por hash real

    if (sqlite3_open(DB_FILE, &db) != SQLITE_OK) {
        fprintf(stderr, "Error DB: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    if (sqlite3_exec(db, sql, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "Error SQL: %s\n", err_msg);
        sqlite3_free(err_msg);
        exit(1);
    }
}

int verificar_credenciales(const char *usuario, const char *contrasena) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT contrasena FROM usuarios WHERE usuario = ?";
    char hash_bd[256] = {0};

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) return 0;

    sqlite3_bind_text(stmt, 1, usuario, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *hash_db = sqlite3_column_text(stmt, 0);
        if (hash_db) {
            strncpy(hash_bd, (const char *)hash_db, sizeof(hash_bd) - 1);
            char *hash_input = crypt(contrasena, hash_bd); // Comparar usando mismo salt
            if (strcmp(hash_input, hash_bd) == 0) {
                sqlite3_finalize(stmt);
                return 1;
            }
        }
    }

    sqlite3_finalize(stmt);
    return 0;
}

void handle_login(struct mg_connection *c, struct mg_http_message *hm) {
    if (mg_vcmp(&hm->method, "GET") == 0) {
        const char *form = "<form method='POST' action='/login'>"
                           "Usuario: <input name='usuario'><br>"
                           "Contraseña: <input name='contrasena' type='password'><br>"
                           "<button type='submit'>Login</button></form>";
        mg_http_reply(c, 200, "Content-Type: text/html\r\n", "%s", form);
    } else if (mg_vcmp(&hm->method, "POST") == 0) {
        char usuario[64], contrasena[64];
        memset(usuario, 0, sizeof(usuario));
        memset(contrasena, 0, sizeof(contrasena));

        if (mg_http_get_var(&hm->body, "usuario", usuario, sizeof(usuario)) < 0 ||
            mg_http_get_var(&hm->body, "contrasena", contrasena, sizeof(contrasena)) < 0) {
            mg_http_reply(c, 400, "", "Parámetros faltantes\n");
            return;
        }

        if (verificar_credenciales(usuario, contrasena)) {
            mg_http_reply(c, 200, "Set-Cookie: token=ok\r\nContent-Type: text/html\r\n",
                          "<p>Login exitoso</p>");
        } else {
            mg_http_reply(c, 401, "Content-Type: text/html\r\n", "<p>Login inválido</p>");
        }
    } else {
        mg_http_reply(c, 405, "", "Método no permitido\n");
    }
}

void cb(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *)ev_data;
        if (mg_http_match_uri(hm, "/login")) {
            handle_login(c, hm);
        } else {
            mg_http_reply(c, 404, "", "Ruta no encontrada\n");
        }
    }
}

int main(void) {
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);

    init_db();
    mg_http_listen(&mgr, "http://localhost:8000", cb, NULL);
    printf("Servidor en http://localhost:8000\n");

    for (;;) mg_mgr_poll(&mgr, 1000);

    sqlite3_close(db);
    mg_mgr_free(&mgr);
    return 0;
}
