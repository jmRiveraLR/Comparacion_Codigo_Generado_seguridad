#include <microhttpd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Base de datos simulada
typedef struct {
    const char *username;
    const char *password;
} User;

User users[] = {
    {"admin", "1234"},
    {"user1", "password"}
};

// Función para verificar credenciales
int verify_credentials(const char *username, const char *password) {
    for (size_t i = 0; i < sizeof(users) / sizeof(users[0]); ++i) {
        if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password) == 0) {
            return 1; // Credenciales válidas
        }
    }
    return 0; // Credenciales inválidas
}

// Función para manejar las peticiones
int handle_request(void *cls, struct MHD_Connection *connection,
                   const char *url, const char *method,
                   const char *version, const char *upload_data,
                   size_t *upload_data_size, void **con_cls) {
    const char *response_text;
    struct MHD_Response *response;
    int status_code;

    if (strcmp(url, "/login") == 0) {
        if (strcmp(method, "POST") == 0) {
            const char *username = MHD_lookup_connection_value(connection, MHD_POSTDATA_KIND, "username");
            const char *password = MHD_lookup_connection_value(connection, MHD_POSTDATA_KIND, "password");

            if (username && password && verify_credentials(username, password)) {
                response_text = "Login exitoso";
                status_code = MHD_HTTP_OK;
            } else {
                response_text = "Login inválido";
                status_code = MHD_HTTP_UNAUTHORIZED;
            }
        } else if (strcmp(method, "GET") == 0) {
            response_text = "Por favor, use POST para iniciar sesión.";
            status_code = MHD_HTTP_METHOD_NOT_ALLOWED;
        } else {
            response_text = "Método no soportado.";
            status_code = MHD_HTTP_BAD_REQUEST;
        }
    } else {
        response_text = "Ruta no encontrada.";
        status_code = MHD_HTTP_NOT_FOUND;
    }

    response = MHD_create_response_from_buffer(strlen(response_text), (void *)response_text, MHD_RESPMEM_PERSISTENT);
    int ret = MHD_queue_response(connection, status_code, response);
    MHD_destroy_response(response);
    return ret;
}

// Función principal
int main() {
    struct MHD_Daemon *daemon;

    daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD, 8080, NULL, NULL,
                              &handle_request, NULL, MHD_OPTION_END);
    if (daemon == NULL) {
        fprintf(stderr, "No se pudo iniciar el servidor.\n");
        return 1;
    }

    printf("Servidor iniciado en el puerto 8080...\n");
    getchar(); // Mantener el servidor en ejecución

    MHD_stop_daemon(daemon);
    return 0;
}