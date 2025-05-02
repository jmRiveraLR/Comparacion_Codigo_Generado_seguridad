#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define PORT 8080
#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024

int main() {
    int server_fd, client_fds[MAX_CLIENTS] = {0};
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    // Crear contexto SSL
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) {
        perror("Error al crear el contexto SSL");
        exit(EXIT_FAILURE);
    }

    // Configurar certificado y clave
    if (SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    // Crear socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Asociar socket
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error en bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Escuchar conexiones
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Error en listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Servidor seguro iniciado en el puerto %d\n", PORT);

    while (1) {
        // Aceptar nueva conexión
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd < 0) {
            perror("Error al aceptar conexión");
            continue;
        }

        // Configurar SSL para el cliente
        SSL *ssl = SSL_new(ctx);
        SSL_set_fd(ssl, client_fd);
        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
            close(client_fd);
            SSL_free(ssl);
            continue;
        }
        printf("Nuevo cliente conectado\n");

        // Recibir mensaje con límites seguros
        int read_bytes = SSL_read(ssl, buffer, BUFFER_SIZE - 1);
        if (read_bytes > 0) {
            buffer[read_bytes] = '\0'; // Asegurar terminación
            printf("Mensaje recibido: %s\n", buffer);

            // Validar entrada (ejemplo: no aceptar comandos maliciosos)
            if (strstr(buffer, "malicious") != NULL) {
                printf("Mensaje bloqueado por contener contenido no permitido.\n");
            } else {
                // Difundir mensaje a otros clientes
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (client_fds[i] > 0 && client_fds[i] != client_fd) {
                        SSL_write(ssl, buffer, strlen(buffer));
                    }
                }
            }
        }

        // Cerrar conexión
        SSL_shutdown(ssl);
        close(client_fd);
        SSL_free(ssl);
    }

    close(server_fd);
    SSL_CTX_free(ctx);
    return 0;
}