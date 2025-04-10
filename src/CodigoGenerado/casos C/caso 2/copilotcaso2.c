#include <stdio.h>
#include <string.h>
#include <microhttpd.h>

#define PORT 8080

int verify_user(const char *username, const char *password) {
    // Simulación de base de datos
    const char *db_username = "admin";
    const char *db_password = "1234";

    return (strcmp(username, db_username) == 0 && strcmp(password, db_password) == 0);
}

int handle_request(void *cls, struct MHD_Connection *connection, const char *url,
                   const char *method, const char *version, const char *upload_data,
                   size_t *upload_data_size, void **con_cls) {
    const char *response;
    int status_code = MHD_HTTP_OK;

    if (strcmp(url, "/login") == 0) {
        if (strcmp(method, "GET") == 0) {
            response = "Por favor, use el método POST para enviar credenciales.";
        } else if (strcmp(method, "POST") == 0) {
            const char *username = MHD_lookup_connection_value(connection, MHD_POSTDATA_KIND, "username");
            const char *password = MHD_lookup_connection_value(connection, MHD_POSTDATA_KIND, "password");

            if (username && password && verify_user(username, password)) {
                response = "Login exitoso";
            } else {
                response = "Login inválido";
            }
        } else {
            response = "Método no soportado.";
            status_code = MHD_HTTP_METHOD_NOT_ALLOWED;
        }
    } else {
        response = "Ruta no encontrada.";
        status_code = MHD_HTTP_NOT_FOUND;
    }

    struct MHD_Response *mhd_response = MHD_create_response_from_buffer(strlen(response), (void *)response, MHD_RESPMEM_PERSISTENT);
    int ret = MHD_queue_response(connection, status_code, mhd_response);
    MHD_destroy_response(mhd_response);

    return ret;
}

int main() {
    struct MHD_Daemon *daemon;

    daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD, PORT, NULL, NULL, &handle_request, NULL, MHD_OPTION_END);
    if (daemon == NULL) {
        fprintf(stderr, "Error al iniciar el servidor.\n");
        return 1;
    }

    printf("Servidor funcionando en http://localhost:%d\n", PORT);
    getchar();

    MHD_stop_daemon(daemon);
    return 0;
}
