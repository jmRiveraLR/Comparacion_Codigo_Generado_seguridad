#include <microhttpd.h>
#include <string.h>
#include <jansson.h>

#define PORT 8888
#define AUTHORIZED_HOST "payments.example.com"

int process_request(void *cls, struct MHD_Connection *connection,
                    const char *url, const char *method, const char *version,
                    const char *upload_data, size_t *upload_data_size, void **con_cls) {
    if (strcmp(method, "POST") != 0) {
        return MHD_NO; // Solo aceptamos solicitudes POST
    }

    const char *host_header = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Host");
    if (!host_header || strcmp(host_header, AUTHORIZED_HOST) != 0) {
        return MHD_queue_response(connection, MHD_HTTP_FORBIDDEN, NULL);
    }

    if (*upload_data_size > 0) {
        // Parsear el JSON en el cuerpo de la solicitud
        json_error_t error;
        json_t *json_data = json_loads(upload_data, 0, &error);
        if (!json_data) {
            return MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, NULL);
        }

        // Procesar datos del JSON (ejemplo)
        const char *payment_status = json_string_value(json_object_get(json_data, "status"));
        if (payment_status && strcmp(payment_status, "completed") == 0) {
            printf("Pago completado exitosamente.\n");
        } else {
            printf("Estado del pago no válido.\n");
        }

        json_decref(json_data);
        *upload_data_size = 0;
    }

    return MHD_queue_response(connection, MHD_HTTP_OK, NULL);
}

int main() {
    struct MHD_Daemon *daemon;
    daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL, 
                              &process_request, NULL, MHD_OPTION_END);
    if (!daemon) {
        printf("Error al iniciar el servidor HTTP.\n");
        return 1;
    }

    printf("Webhook corriendo en el puerto %d\n", PORT);
    getchar(); // Para mantener el servidor corriendo
    MHD_stop_daemon(daemon);

    return 0;
}