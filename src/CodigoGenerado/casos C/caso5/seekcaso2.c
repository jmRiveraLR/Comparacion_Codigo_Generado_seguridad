#include <microhttpd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libpq-fe.h>

#define PORT 8080

// Estructura para pasar datos a los callbacks
struct connection_info {
    char *username;
    char *password;
    int login_success;
};

// Función para verificar credenciales en la base de datos
int check_credentials(const char *username, const char *password) {
    PGconn *conn = PQconnectdb("dbname=usuarios user=postgres password=tu_password");
    
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Error de conexión: %s\n", PQerrorMessage(conn));
        PQfinish(conn);
        return 0;
    }
    
    char query[256];
    snprintf(query, sizeof(query), 
             "SELECT 1 FROM usuarios WHERE username = '%s' AND password = '%s'", 
             username, password);
    
    PGresult *res = PQexec(conn, query);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Error en consulta: %s\n", PQerrorMessage(conn));
        PQclear(res);
        PQfinish(conn);
        return 0;
    }
    
    int rows = PQntuples(res);
    PQclear(res);
    PQfinish(conn);
    
    return rows > 0;
}

// Callback para manejar las solicitudes POST
static int post_iterator(void *cls, enum MHD_ValueKind kind, 
                         const char *key, const char *filename,
                         const char *content_type, const char *transfer_encoding,
                         const char *data, uint64_t off, size_t size) {
    struct connection_info *con_info = cls;
    
    if (strcmp(key, "username") == 0) {
        con_info->username = strndup(data, size);
    } else if (strcmp(key, "password") == 0) {
        con_info->password = strndup(data, size);
    }
    
    return MHD_YES;
}

// Callback principal para manejar las solicitudes
static int answer_to_connection(void *cls, struct MHD_Connection *connection,
                               const char *url, const char *method,
                               const char *version, const char *upload_data,
                               size_t *upload_data_size, void **con_cls) {
    
    if (strcmp(url, "/login") != 0) {
        const char *page = "<html><body>Ruta no encontrada</body></html>";
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(page), 
                                                                       (void*)page, 
                                                                       MHD_RESPMEM_PERSISTENT);
        int ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
        MHD_destroy_response(response);
        return ret;
    }
    
    if (strcmp(method, "GET") == 0) {
        // Mostrar formulario de login para GET
        const char *page = "<html><body>"
                           "<h1>Login</h1>"
                           "<form method=\"post\" action=\"/login\">"
                           "Usuario: <input type=\"text\" name=\"username\"><br>"
                           "Contraseña: <input type=\"password\" name=\"password\"><br>"
                           "<input type=\"submit\" value=\"Login\">"
                           "</form>"
                           "</body></html>";
        
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(page), 
                                                                       (void*)page, 
                                                                       MHD_RESPMEM_PERSISTENT);
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }
    else if (strcmp(method, "POST") == 0) {
        struct connection_info *con_info;
        
        if (*con_cls == NULL) {
            con_info = malloc(sizeof(struct connection_info));
            con_info->username = NULL;
            con_info->password = NULL;
            con_info->login_success = 0;
            *con_cls = con_info;
            return MHD_YES;
        }
        
        con_info = *con_cls;
        
        if (*upload_data_size != 0) {
            MHD_post_process(con_info, upload_data, *upload_data_size);
            *upload_data_size = 0;
            return MHD_YES;
        }
        
        // Verificar credenciales
        if (con_info->username && con_info->password) {
            con_info->login_success = check_credentials(con_info->username, con_info->password);
        }
        
        // Construir respuesta
        const char *page;
        if (con_info->login_success) {
            page = "<html><body><h1>Login exitoso</h1></body></html>";
        } else {
            page = "<html><body><h1>Login inválido</h1></body></html>";
        }
        
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(page), 
                                                                       (void*)page, 
                                                                       MHD_RESPMEM_PERSISTENT);
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        
        // Liberar memoria
        if (con_info->username) free(con_info->username);
        if (con_info->password) free(con_info->password);
        free(con_info);
        *con_cls = NULL;
        
        return ret;
    }
    else {
        // Método no soportado
        const char *page = "<html><body>Método no soportado</body></html>";
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(page), 
                                                                       (void*)page, 
                                                                       MHD_RESPMEM_PERSISTENT);
        int ret = MHD_queue_response(connection, MHD_HTTP_METHOD_NOT_ALLOWED, response);
        MHD_destroy_response(response);
        return ret;
    }
}

int main() {
    struct MHD_Daemon *daemon;
    
    daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD, PORT, NULL, NULL,
                              &answer_to_connection, NULL, MHD_OPTION_END);
    if (NULL == daemon) {
        fprintf(stderr, "Error al iniciar el servidor\n");
        return 1;
    }
    
    printf("Servidor escuchando en el puerto %d...\n", PORT);
    getchar();
    
    MHD_stop_daemon(daemon);
    return 0;
}