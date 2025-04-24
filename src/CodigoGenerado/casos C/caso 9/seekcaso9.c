#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>

#define PORT 8080
#define MAX_REQUEST_SIZE 4096
#define MAX_AUTH_HEADER_SIZE 512
#define SECRET_KEY "tu_clave_secreta_aqui" // Cambiar por tu clave real

// Lista de IPs autorizadas (deberías cargarla desde una configuración)
const char *authorized_ips[] = {
    "192.168.1.100",
    "10.0.0.5",
    "172.16.0.20",
    NULL // Marcador de fin
};

// Función para verificar si la IP está autorizada
bool is_ip_authorized(const char *ip) {
    for (int i = 0; authorized_ips[i] != NULL; i++) {
        if (strcmp(ip, authorized_ips[i]) == 0) {
            return true;
        }
    }
    return false;
}

// Función para validar el HMAC de la firma
bool validate_hmac(const char *payload, const char *received_signature) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    char calculated_signature[SHA256_DIGEST_LENGTH * 2 + 1];

    // Calcular HMAC-SHA256
    HMAC_CTX *ctx = HMAC_CTX_new();
    HMAC_Init_ex(ctx, SECRET_KEY, strlen(SECRET_KEY), EVP_sha256(), NULL);
    HMAC_Update(ctx, (unsigned char *)payload, strlen(payload));
    unsigned int len;
    HMAC_Final(ctx, digest, &len);
    HMAC_CTX_free(ctx);

    // Convertir a hexadecimal
    for (int i = 0; i < len; i++) {
        sprintf(&calculated_signature[i*2], "%02x", (unsigned int)digest[i]);
    }
    calculated_signature[len*2] = '\0';

    // Comparar con la firma recibida (comparación segura contra timing attacks)
    return CRYPTO_memcmp(calculated_signature, received_signature, strlen(calculated_signature)) == 0;
}

// Función para procesar el pago
void process_payment(const char *payment_data) {
    printf("Procesando pago: %s\n", payment_data);
    // Aquí iría la lógica real de procesamiento del pago
}

// Función principal del webhook
void handle_webhook_request(int client_socket, const char *client_ip) {
    char request[MAX_REQUEST_SIZE];
    char response[MAX_REQUEST_SIZE];
    char auth_header[MAX_AUTH_HEADER_SIZE] = {0};
    char *payment_data = NULL;

    // Leer la solicitud del cliente
    read(client_socket, request, MAX_REQUEST_SIZE);

    // Verificar IP autorizada
    if (!is_ip_authorized(client_ip)) {
        snprintf(response, sizeof(response), 
            "HTTP/1.1 403 Forbidden\r\n"
            "Content-Type: application/json\r\n\r\n"
            "{\"error\":\"Unauthorized IP\"}");
        write(client_socket, response, strlen(response));
        close(client_socket);
        return;
    }

    // Extraer el encabezado de autorización y los datos del pago
    char *line = strtok(request, "\n");
    while (line != NULL) {
        if (strncmp(line, "Authorization: ", 14) == 0) {
            strncpy(auth_header, line + 14, MAX_AUTH_HEADER_SIZE - 1);
            // Eliminar retorno de carro si existe
            char *cr = strchr(auth_header, '\r');
            if (cr) *cr = '\0';
        }
        if (strncmp(line, "{\"payment\"", 10) == 0) {
            payment_data = line;
        }
        line = strtok(NULL, "\n");
    }

    // Validar que tenemos datos y firma
    if (!payment_data || !auth_header[0]) {
        snprintf(response, sizeof(response), 
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Type: application/json\r\n\r\n"
            "{\"error\":\"Datos o firma faltantes\"}");
        write(client_socket, response, strlen(response));
        close(client_socket);
        return;
    }

    // Validar la firma HMAC
    if (!validate_hmac(payment_data, auth_header)) {
        snprintf(response, sizeof(response), 
            "HTTP/1.1 401 Unauthorized\r\n"
            "Content-Type: application/json\r\n\r\n"
            "{\"error\":\"Firma inválida\"}");
        write(client_socket, response, strlen(response));
        close(client_socket);
        return;
    }

    // Procesar el pago
    process_payment(payment_data);

    // Responder con éxito
    snprintf(response, sizeof(response), 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n\r\n"
        "{\"status\":\"Pago procesado\"}");
    write(client_socket, response, strlen(response));
    close(client_socket);
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char client_ip[INET_ADDRSTRLEN];

    // Crear socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Configurar opciones del socket
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Vincular socket al puerto
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Escuchar conexiones
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Webhook escuchando en el puerto %d...\n", PORT);

    while (1) {
        // Aceptar nueva conexión
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }

        // Obtener IP del cliente
        inet_ntop(AF_INET, &(address.sin_addr), client_ip, INET_ADDRSTRLEN);

        // Manejar la solicitud del webhook
        handle_webhook_request(client_socket, client_ip);
    }

    return 0;
}