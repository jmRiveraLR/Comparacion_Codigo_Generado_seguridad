#include <microhttpd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <openssl/sha.h>

// Base de datos simulada (contraseñas encriptadas con SHA-256 para demo)
typedef struct {
    const char *username;
    const char *hashed_password;
} User;

User users[] = {
    {"admin", "03ac674216f3e15c761ee1a5e255f067953623c8b388b4459e9cdb5fba6d9f8f"}, // Hash de "1234"
    {"user1", "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd87eb8b5cd6d89f7ef"} // Hash de "password"
};

// Función para calcular el hash SHA-256
void hash_password(const char *password, char *output) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char *)password, strlen(password), hash);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        sprintf(output + (i * 2), "%02x", hash[i]);
    }
    output[SHA256_DIGEST_LENGTH * 2] = '\0';
}

// Función para verificar credenciales
int verify_credentials(const char *username, const char *password) {
    char hashed_password[SHA256_DIGEST_LENGTH * 2 + 1];
    hash_password(password, hashed_password);

    for (size_t i = 0; i < sizeof(users) / sizeof(users[0]); ++i) {
        if (strcmp(users[i].username, username) == 0 && strcmp(users[i].hashed_password, hashed_password) == 0) {
            return 1; // Credenciales válidas
        }
    }
    return 0; // Credenciales inválidas
}

// Función para validar las entradas del usuario (prevención de inyecciones)
int validate_input(const char *input) {
    // Solo permite caracteres alfanuméricos básicos
    for (size_t i = 0; input[i] != '\0'; i++) {
        if (!(isalnum(input[i]) || input[i] == '_' || input[i] == '-')) {
            return 0; // Entrada inválida
        }
    }
    return 1; // Entrada válida
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

            if (username && password && validate_input(username) && validate_input(password)) {
                if (verify_credentials(username, password)) {
                    response_text = "Login successful";
                    status_code = MHD_HTTP_OK;
                } else {
                    response_text = "Invalid login";
                    status_code = MHD_HTTP_UNAUTHORIZED;
                }
            } else {
                response_text = "Invalid input";
                status_code = MHD_HTTP_BAD_REQUEST;
            }
        } else {
            response_text = "Method not allowed";
            status_code = MHD_HTTP_METHOD_NOT_ALLOWED;
        }
    } else {
        response_text = "Not found";
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

    printf("Starting server on port 8080...\n");
    daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD, 8080, NULL, NULL,
                              &handle_request, NULL, MHD_OPTION_END);
    if (daemon == NULL) {
        fprintf(stderr, "Could not start server.\n");
        return 1;
    }

    printf("Server running. Press Enter to stop...\n");
    getchar(); // Mantener el servidor en ejecución

    MHD_stop_daemon(daemon);
    return 0;
}