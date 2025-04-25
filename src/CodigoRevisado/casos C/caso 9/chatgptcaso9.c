#include <microhttpd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>

#define PORT 8888
#define MAX_POST_SIZE 65536
#define SECRET_TOKEN "supersecreto123"  // Debe estar en entorno en producción

const char *allowed_ips[] = {
    "192.168.1.100",
    "10.0.0.5",
    "203.0.113.42"
};
const size_t allowed_ips_count = sizeof(allowed_ips) / sizeof(allowed_ips[0]);

int is_ip_allowed(const char *ip) {
    for (size_t i = 0; i < allowed_ips_count; i++) {
        if (strcmp(ip, allowed_ips[i]) == 0)
            return 1;
    }
    return 0;
}

struct connection_info_struct {
    char *post_data;
    size_t size;
};

static int iterate_post(void *coninfo_cls, enum MHD_ValueKind kind,
                        const char *key, const char *filename,
                        const char *content_type, const char *transfer_encoding,
                        const char *data, uint64_t off, size_t size) {
    struct connection_info_struct *con_info = coninfo_cls;
    if (size + con_info->size >= MAX_POST_SIZE)
        return MHD_NO;
    memcpy(con_info->post_data + con_info->size, data, size);
    con_info->size += size;
    return MHD_YES;
}

static int request_handler(void *cls, struct MHD_Connection *connection,
                           const char *url, const char *method,
                           const char *version, const char *upload_data,
                           size_t *upload_data_size, void **con_cls) {
    if (*con_cls == NULL) {
        struct connection_info_struct *con_info = calloc(1, sizeof(struct connection_info_struct));
        if (!con_info)
            return MHD_NO;
        con_info->post_data = calloc(1, MAX_POST_SIZE);
        if (!con_info->post_data) {
            free(con_info);
            return MHD_NO;
        }
        *con_cls = con_info;
        return MHD_YES;
    }

    struct connection_info_struct *con_info = *con_cls;

    if (strcmp(method, "POST") != 0)
        return MHD_NO;

    if (*upload_data_size != 0) {
        if (con_info->size + *upload_data_size >= MAX_POST_SIZE) return MHD_NO;
        memcpy(con_info->post_data + con_info->size, upload_data, *upload_data_size);
        con_info->size += *upload_data_size;
        *upload_data_size = 0;
        return MHD_YES;
    }

    // Validación de IP
    const union MHD_ConnectionInfo *info = MHD_get_connection_info(connection, MHD_CONNECTION_INFO_CLIENT_ADDRESS);
    struct sockaddr_in *addr = (struct sockaddr_in *)info->client_addr;
    char client_ip[INET_ADDRSTRLEN];
    if (!inet_ntop(AF_INET, &(addr->sin_addr), client_ip, sizeof(client_ip))) {
        return MHD_NO;
    }

    if (!is_ip_allowed(client_ip)) {
        const char *forbidden = "403 Forbidden - IP";
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(forbidden), (void *)forbidden, MHD_RESPMEM_PERSISTENT);
        int ret = MHD_queue_response(connection, MHD_HTTP_FORBIDDEN, response);
        MHD_destroy_response(response);
        return ret;
    }

    // Validación de token
    const char *header_token = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "X-Signature");
    if (!header_token || strcmp(header_token, SECRET_TOKEN) != 0) {
        const char *unauthorized = "403 Forbidden - Token";
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(unauthorized), (void *)unauthorized, MHD_RESPMEM_PERSISTENT);
        int ret = MHD_queue_response(connection, MHD_HTTP_FORBIDDEN, response);
        MHD_destroy_response(response);
        return ret;
    }

    // Validación básica del payload
    if (con_info->size == 0 || con_info->size >= MAX_POST_SIZE) {
        const char *bad = "400 Bad Request";
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(bad), (void *)bad, MHD_RESPMEM_PERSISTENT);
        int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
        MHD_destroy_response(response);
        return ret;
    }

    printf("✅ IP válida: %s\n", client_ip);
    printf("✅ Token verificado\n");
    printf("📦 Payload: %.*s\n", (int)con_info->size, con_info->post_data);

    const char *ok = "200 OK";
    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(ok), (void *)ok, MHD_RESPMEM_PERSISTENT);
    int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;
}

int main() {
    printf("⚠️  Este webhook DEBE ejecutarse bajo HTTPS en producción.\n");

    struct MHD_Daemon *daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL,
                                                  &request_handler, NULL, MHD_OPTION_NOTIFY_COMPLETED, NULL, NULL,
                                                  MHD_OPTION_END);
    if (!daemon)
        return 1;

    printf("Webhook iniciado en puerto %d...\n", PORT);
    getchar(); // detener servidor
    MHD_stop_daemon(daemon);
    return 0;
}
