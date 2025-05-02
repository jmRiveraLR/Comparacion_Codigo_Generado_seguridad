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
#include <time.h>

#define PORT 8080
#define MAX_REQUEST_SIZE 4096
#define MAX_AUTH_HEADER_SIZE 512
#define MAX_TIMESTAMP_DIFF 300 // 5 minutos en segundos
#define SECRET_KEY_FILE "/etc/webhook_secret.key" // Archivo externo para clave

// Lista de IPs autorizadas (mejor cargarla desde archivo de configuración)
const char *authorized_ips[] = {
    "192.168.1.100",
    "10.0.0.5",
    "172.16.0.20",
    NULL
};

// Función segura para leer la clave secreta
bool read_secret_key(char *buffer, size_t buffer_size) {
    FILE *file = fopen(SECRET_KEY_FILE, "r");
    if (!file) return false;
    
    if (!fgets(buffer, buffer_size, file)) {
        fclose(file);
        return false;
    }
    
    // Eliminar newline si existe
    char *newline = strchr(buffer, '\n');
    if (newline) *newline = '\0';
    
    fclose(file);
    return true;
}

// Función para verificar IP autorizada
bool is_ip_authorized(const char *ip) {
    if (!ip) return false;
    
    for (int i = 0; authorized_ips[i] != NULL; i++) {
        if (strcmp(ip, authorized_ips[i]) == 0) {
            return true;
        }
    }
    return false;
}

// Función segura para validar HMAC
bool validate_hmac(const char *payload, const char *received_signature, const char *timestamp) {
    if (!payload || !received_signature || !timestamp) return false;

    char secret_key[256];
    if (!read_secret_key(secret_key, sizeof(secret_key))) {
        return false;
    }

    // Validar timestamp (prevent replay attacks)
    time_t now = time(NULL);
    time_t req_time = atol(timestamp);
    if (abs(now - req_time) > MAX_TIMESTAMP_DIFF) {
        return false;
    }

    unsigned char digest[SHA256_DIGEST_LENGTH];
    char calculated_signature[SHA256_DIGEST_LENGTH * 2 + 1];

    // Calcular HMAC-SHA256 de forma segura
    HMAC_CTX *ctx = HMAC_CTX_new();
    if (!ctx) return false;
    
    if (!HMAC_Init_ex(ctx, secret_key, strlen(secret_key), EVP_sha256(), NULL) ||
        !HMAC_Update(ctx, (unsigned char *)timestamp, strlen(timestamp)) ||
        !HMAC_Update(ctx, (unsigned char *)payload, strlen(payload))) {
        HMAC_CTX_free(ctx);
        return false;
    }
    
    unsigned int len;
    if (!HMAC_Final(ctx, digest, &len)) {
        HMAC_CTX_free(ctx);
        return false;
    }
    HMAC_CTX_free(ctx);

    // Convertir a hexadecimal de forma segura
    for (int i = 0; i < len && i*2 < sizeof(calculated_signature)-1; i++) {
        snprintf(&calculated_signature[i*2], 3, "%02x", (unsigned int)digest[i]);
    }
    calculated_signature[len*2] = '\0';

    // Comparación segura contra timing attacks
    return CRYPTO_memcmp(calculated_signature, received_signature, strlen(calculated_signature)) == 0;
}

// Función segura para extraer encabezado
bool extract_header(const char *request, const char *header_name, char *output, size_t output_size) {
    char *line = strstr(request, header_name);
    if (!line) return false;
    
    line += strlen(header_name);
    char *end = strchr(line, '\r');
    if (!end) end = strchr(line, '\n');
    if (!end) return false;
    
    size_t length = end - line;
    if (length >= output_size) return false;
    
    strncpy(output, line, length);
    output[length] = '\0';
    return true;
}

// Función segura para procesar solicitud
void handle_webhook_request(int client_socket, const char *client_ip) {
    char request[MAX_REQUEST_SIZE + 1] = {0}; // +1 para null terminator
    char response[MAX_REQUEST_SIZE];
    char auth_header[MAX_AUTH_HEADER_SIZE] = {0};
    char timestamp_header[MAX_AUTH_HEADER_SIZE] = {0};
    char *payment_data = NULL;

    // Leer solicitud de forma segura
    ssize_t bytes_read = read(client_socket, request, MAX_REQUEST_SIZE);
    if (bytes_read <= 0) {
        snprintf(response, sizeof(response), "HTTP/1.1 400 Bad Request\r\n\r\n");
        write(client_socket, response, strlen(response));
        close(client_socket);
        return;
    }
    request[bytes_read] = '\0';

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

    // Extraer encabezados de forma segura
    if (!extract_header(request, "X-Signature: ", auth_header, sizeof(auth_header)) ||
        !extract_header(request, "X-Timestamp: ", timestamp_header, sizeof(timestamp_header))) {
        snprintf(response, sizeof(response), 
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Type: application/json\r\n\r\n"
            "{\"error\":\"Missing required headers\"}");
        write(client_socket, response, strlen(response));
        close(client_socket);
        return;
    }

    // Buscar datos del pago (validar formato JSON)
    payment_data = strstr(request, "\r\n\r\n");
    if (!payment_data || strlen(payment_data + 4) == 0) {
        payment_data = strstr(request, "\n\n");
        if (!payment_data || strlen(payment_data + 2) == 0) {
            snprintf(response, sizeof(response), 
                "HTTP/1.1 400 Bad Request\r\n"
                "Content-Type: application/json\r\n\r\n"
                "{\"error\":\"Invalid payload\"}");
            write(client_socket, response, strlen(response));
            close(client_socket);
            return;
        }
        payment_data += 2;
    } else {
        payment_data += 4;
    }

    // Validar HMAC con timestamp
    if (!validate_hmac(payment_data, auth_header, timestamp_header)) {
        snprintf(response, sizeof(response), 
            "HTTP/1.1 401 Unauthorized\r\n"
            "Content-Type: application/json\r\n\r\n"
            "{\"error\":\"Invalid signature\"}");
        write(client_socket, response, strlen(response));
        close(client_socket);
        return;
    }

    // Procesar pago (aquí iría la lógica real)
    printf("Procesando pago válido de %s\n", client_ip);

    // Responder
    snprintf(response, sizeof(response), 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n\r\n"
        "{\"status\":\"success\"}");
    write(client_socket, response, strlen(response));
    close(client_socket);
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char client_ip[INET_ADDRSTRLEN];

    // Verificar privilegios
    if (geteuid() == 0) {
        fprintf(stderr, "Error: No se debe ejecutar como root\n");
        exit(EXIT_FAILURE);
    }

    // Crear socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Configurar opciones del socket de forma segura
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Vincular socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Escuchar
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Webhook seguro escuchando en puerto %d...\n", PORT);

    while (1) {
        // Aceptar conexión
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }

        // Obtener IP del cliente
        inet_ntop(AF_INET, &(address.sin_addr), client_ip, INET_ADDRSTRLEN);
        
        // Manejar solicitud
        handle_webhook_request(client_socket, client_ip);
    }

    close(server_fd);
    return 0;
}