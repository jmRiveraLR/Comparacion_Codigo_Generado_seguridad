#include "mongoose.h"
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

#define DB_FILE "usuarios.db"

static sqlite3 *db;

static void init_db() {
    char *err_msg = NULL;
    const char *sql = "CREATE TABLE IF NOT EXISTS usuarios ("
                      "usuario TEXT PRIMARY KEY,"
                      "contrasena TEXT);"
                      "INSERT OR IGNORE INTO usuarios (usuario, contrasena) VALUES ('admin', '1234');";

    if (sqlite3_open(DB_FILE, &db) != SQLITE_OK) {
        fprintf(stderr, "No se pudo abrir la base de datos: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    if (sqlite3_exec(db, sql, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "Error al crear tabla: %s\n", err_msg);
        sqlite3_free(err_msg);
        exit(1);
    }
}

int verificar_credenciales(const char *usuario, const char *contrasena) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT * FROM usuarios WHERE usuario = ? AND contrasena = ?";
    int encontrado = 0;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, usuario, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, contrasena, -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            encontrado = 1;
        }
    }

    sqlite3_finalize(stmt);
    return encontrado;
}

static void handle_login(struct mg_connection *c, struct mg_http_message *hm) {
    if (mg_vcmp(&hm->method, "GET") == 0) {
        const char *form = "<form method='POST' action='/login'>"
                           "Usuario: <input name='usuario'><br>"
                           "Contraseña: <input name='contrasena' type='password'><br>"
                           "<button type='submit'>Login</button></form>";
        mg_http_reply(c, 200, "Content-Type: text/html\r\n", "%s", form);
    } else if (mg_vcmp(&hm->method, "POST") == 0) {
        char usuario[100], contrasena[100];
        mg_http_get_var(&hm->body, "usuario", usuario, sizeof(usuario));
        mg_http_get_var(&hm->body, "contrasena", contrasena, sizeof(contrasena));

        if (verificar_credenciales(usuario, contrasena)) {
            mg_http_reply(c, 200, "", "Login exitoso!\n");
        } else {
            mg_http_reply(c, 401, "", "Login inválido\n");
        }
    } else {
        mg_http_reply(c, 405, "", "Método no permitido\n");
    }
}

static void cb(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
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
    printf("Servidor corriendo en http://localhost:8000\n");

    for (;;) mg_mgr_poll(&mgr, 1000);
    sqlite3_close(db);
    mg_mgr_free(&mgr);
    return 0;
}
