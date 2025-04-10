#include <microhttpd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#define PORT 8080

// Estructura para pasar datos a los callbacks
struct connection_info {
    char *username;
    char *password;
    int login_success;
};

// Función para manejar las solicitudes POST
static int iterate_post(void *coninfo_cls, enum MHD_ValueKind kind, 
                        const char *key, const char *filename,
                        const char *content_type, const char *transfer_encoding,
                        const char *data, uint64_t off, size_t size) {
    struct connection_info *con_info = coninfo_cls;
    
    if (0 == strcmp(key, "username")) {
        con_info->username = strndup(data, size);
    } else if (0 == strcmp(key, "password")) {
        con_info->password = strndup(data, size);
    }
    
    return MHD_YES;
}

// Función para verificar las credenciales en la base de datos
int verify_credentials(const char *username, const char *password) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;
    int valid = 0;
    
    rc = sqlite3_open("usuarios.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "No se pudo abrir la base de datos: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    
    const char *sql = "SELECT 1 FROM usuarios WHERE username = ? AND password = ?";
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error al preparar la consulta: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 0;
    }
    
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        valid = 1;
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    
    return valid;
}

// Función para generar la respuesta HTTP
static int send_response(struct connection_info *con_info, struct MHD_Response *response, 
                         struct MHD_Connection *connection) {
    const char *page;
    char buffer[1024];
    
    if (con_info->login_success) {
        page = "<html><body><h1>Login exitoso!</h1></body></html>";
    } else {
        page = "<html><body><h1>Login invalido</h1></body></html>";
    }
    
    snprintf(buffer, sizeof(buffer), page);
    response = MHD_create_response_from_buffer(strlen(buffer), (void *)buffer, 
                                              MHD_RESPMEM_MUST_COPY);
    int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    
    return ret;
}

// Callback principal para manejar las solicitudes
static int answer_to_connection(void *cls, struct MHD_Connection *connection,
                               const char *url, const char *method,
                               const char *version, const char *upload_data,
                               size_t *upload_data_size, void **con_cls) {
    struct MHD_Response *response = NULL;
    int ret;
    
    if (strcmp(url, "/login") != 0) {
        const char *page = "<html><body><h1>Pagina no encontrada</h1></body></html>";
        response = MHD_create_response_from_buffer(strlen(page), (void *)page, 
                                                  MHD_RESPMEM_PERSISTENT);
        ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
        MHD_destroy_response(response);
        return ret;
    }
    
    if (*con_cls == NULL) {
        // Primera llamada para esta conexión
        struct connection_info *con_info = malloc(sizeof(struct connection_info));
        if (con_info == NULL) {
            return MHD_NO;
        }
        con_info->username = NULL;
        con_info->password = NULL;
        con_info->login_success = 0;
        *con_cls = con_info;
        
        if (strcmp(method, "POST") == 0) {
            return MHD_YES;
        } else {
            // Mostrar formulario para GET
            const char *page = "<html><body>"
                              "<form action=\"/login\" method=\"post\">"
                              "Usuario: <input type=\"text\" name=\"username\"><br>"
                              "Contraseña: <input type=\"password\" name=\"password\"><br>"
                              "<input type=\"submit\" value=\"Login\">"
                              "</form></body></html>";
            response = MHD_create_response_from_buffer(strlen(page), (void *)page, 
                                                      MHD_RESPMEM_PERSISTENT);
            ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
            MHD_destroy_response(response);
            return ret;
        }
    }
    
    struct connection_info *con_info = *con_cls;
    
    if (strcmp(method, "POST") == 0) {
        if (*upload_data_size != 0) {
            MHD_post_process(con_info, upload_data, *upload_data_size);
            *upload_data_size = 0;
            return MHD_YES;
        } else {
            // Verificar credenciales
            con_info->login_success = verify_credentials(con_info->username, con_info->password);
            return send_response(con_info, response, connection);
        }
    } else if (strcmp(method, "GET") == 0) {
        // Mostrar formulario para GET
        const char *page = "<html><body>"
                          "<form action=\"/login\" method=\"post\">"
                          "Usuario: <input type=\"text\" name=\"username\"><br>"
                          "Contraseña: <input type=\"password\" name=\"password\"><br>"
                          "<input type=\"submit\" value=\"Login\">"
                          "</form></body></html>";
        response = MHD_create_response_from_buffer(strlen(page), (void *)page, 
                                                  MHD_RESPMEM_PERSISTENT);
        ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }
    
    return MHD_NO;
}

// Función para liberar la memoria de la estructura de conexión
void request_completed(void *cls, struct MHD_Connection *connection,
                      void **con_cls, enum MHD_RequestTerminationCode toe) {
    struct connection_info *con_info = *con_cls;
    if (con_info == NULL) {
        return;
    }
    
    if (con_info->username) {
        free(con_info->username);
    }
    if (con_info->password) {
        free(con_info->password);
    }
    free(con_info);
    *con_cls = NULL;
}

int main() {
    // Crear la base de datos si no existe
    sqlite3 *db;
    int rc = sqlite3_open("usuarios.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "No se pudo abrir/crear la base de datos: %s\n", sqlite3_errmsg(db));
        return 1;
    }
    
    // Crear tabla de usuarios si no existe
    const char *sql = "CREATE TABLE IF NOT EXISTS usuarios ("
                      "username TEXT PRIMARY KEY,"
                      "password TEXT NOT NULL);"
                      "INSERT OR IGNORE INTO usuarios VALUES ('admin', 'admin123');"
                      "INSERT OR IGNORE INTO usuarios VALUES ('user', 'password');";
    
    char *err_msg = NULL;
    rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error al crear la tabla: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }
    sqlite3_close(db);
    
    // Iniciar el servidor web
    struct MHD_Daemon *daemon;
    daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL,
                             &answer_to_connection, NULL,
                             MHD_OPTION_NOTIFY_COMPLETED, &request_completed, NULL,
                             MHD_OPTION_END);
    if (daemon == NULL) {
        fprintf(stderr, "No se pudo iniciar el servidor\n");
        return 1;
    }
    
    printf("Servidor corriendo en http://localhost:%d/login\n", PORT);
    printf("Usuarios de prueba:\n");
    printf("admin / admin123\n");
    printf("user / password\n");
    getchar();
    
    MHD_stop_daemon(daemon);
    return 0;
}