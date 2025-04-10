#include <stdio.h>
#include <string.h>
#include <microhttpd.h>
#include <stdlib.h>

#define PORT 8080

// Simulador de verificación de usuario con mejores prácticas
int verify_user(const char *username, const char *password) {
    // Reemplazar con una base de datos segura o un método cifrado
    const char *db_username = getenv("DB_USERNAME"); // Obtener usuario de variables de entorno
    const char *db_password = getenv("DB_PASSWORD"); // Obtener contraseña de variables de entorno

    if (!db_username || !db_password) {
        return 0; // Error si las variables de entorno no están configuradas
    }

    return (strcmp(username, db_username) == 0 && strcmp(password, db_password) == 0);
}

int handle_request(void *cls, struct MHD_Connection *connection, const char *url,
                   const char *method, const char *version, const char *upload_data,
                   size_t *upload_data_size, void **con_cls) {
    const char *response;
    int status_code = MHD_HTTP_OK;

    // Validar URL y método
    if (strcmp(url, "/login") == 0) {
        if (strcmp(method, "POST") == 0) {
            // Validar si las credenciales están presentes
            const char *username = MHD_lookup_connection_value(connection, MHD_POSTDATA_KIND, "username");
            const char *password = MHD_lookup_connection_value(connection, MHD_POSTDATA_KIND, "password");

            if (username && password && strlen(username) < 256 && strlen(password) < 256) { // Limitar tamaño
                if (verify_user(username, password)) {
                    response = "Login exitoso";
                } else {
                    response = "Login inválido";
                }
            } else {
                response = "Entrada inválida o demasiado larga.";
                status_code = MHD_HTTP_BAD_REQUEST;
            }
        } else {
            response = "Método no soportado. Use POST para credenciales.";
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

    // Imprimir advertencia si las variables de entorno no están configuradas
    if (!getenv("DB_USERNAME") || !getenv("DB_PASSWORD")) {
        fprintf(stderr, "Error: Configure las variables de entorno DB_USERNAME y DB_PASSWORD.\n");
        return 1;
    }

    daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_TLS, PORT, NULL, NULL, &handle_request, NULL, MHD_OPTION_END);
    if (daemon == NULL) {
        fprintf(stderr, "Error al iniciar el servidor.\n");
        return 1;
    }

    printf("Servidor funcionando de forma segura en https://localhost:%d\n", PORT);
    getchar();

    MHD_stop_daemon(daemon);
    return 0;
}