#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <limits.h>

#define BUFFER_SIZE 1024
#define MAX_MSG_SIZE (BUFFER_SIZE - 1)

void *receive_messages(void *arg) {
    int sock = *(int *)arg;
    char buffer[BUFFER_SIZE];
    ssize_t read_size;

    while ((read_size = recv(sock, buffer, MAX_MSG_SIZE, 0)) > 0) {
        buffer[read_size] = '\0'; // Asegurar terminación nula
        printf("%s", buffer);
        fflush(stdout);
    }

    printf("\nConexión con el servidor terminada\n");
    return NULL;
}

int parse_port(const char *port_str) {
    char *endptr;
    long port = strtol(port_str, &endptr, 10);
    
    if (*endptr != '\0' || port < 1 || port > 65535) {
        fprintf(stderr, "Puerto inválido. Debe estar entre 1 y 65535\n");
        return -1;
    }
    
    return (int)port;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <dirección IP> <puerto>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    pthread_t recv_thread;

    // Validar dirección IP
    struct in_addr addr;
    if (inet_pton(AF_INET, argv[1], &addr) <= 0) {
        fprintf(stderr, "Dirección IP inválida\n");
        return EXIT_FAILURE;
    }

    // Validar puerto
    int port = parse_port(argv[2]);
    if (port == -1) {
        return EXIT_FAILURE;
    }

    // Crear socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error al crear el socket");
        return EXIT_FAILURE;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
        perror("Dirección inválida / no soportada");
        close(sock);
        return EXIT_FAILURE;
    }

    // Conectar al servidor con timeout
    fd_set fdset;
    struct timeval tv;
    int so_error;
    socklen_t len = sizeof(so_error);

    // Configurar socket como no bloqueante
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));

    FD_ZERO(&fdset);
    FD_SET(sock, &fdset);
    tv.tv_sec = 5;  // Timeout de 5 segundos
    tv.tv_usec = 0;

    if (select(sock + 1, NULL, &fdset, NULL, &tv) == 1) {
        getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);
        if (so_error != 0) {
            fprintf(stderr, "Error al conectar: %s\n", strerror(so_error));
            close(sock);
            return EXIT_FAILURE;
        }
    } else {
        fprintf(stderr, "Timeout al conectar con el servidor\n");
        close(sock);
        return EXIT_FAILURE;
    }

    // Restaurar modo bloqueante
    fcntl(sock, F_SETFL, flags);

    printf("Conectado al servidor. Escribe mensajes (escribe 'salir' para terminar):\n");

    // Crear hilo para recibir mensajes
    if (pthread_create(&recv_thread, NULL, receive_messages, (void *)&sock)) {
        perror("Error al crear el hilo de recepción");
        close(sock);
        return EXIT_FAILURE;
    }

    // Leer mensajes del usuario y enviarlos al servidor
    while (1) {
        if (!fgets(buffer, BUFFER_SIZE, stdin)) {
            break;
        }

        size_t len = strnlen(buffer, MAX_MSG_SIZE);
        if (len == 0) {
            continue;
        }

        if (strncmp(buffer, "salir\n", 6) == 0) {
            break;
        }

        size_t sent = 0;
        while (sent < len) {
            ssize_t n = send(sock, buffer + sent, len - sent, 0);
            if (n <= 0) {
                perror("Error al enviar mensaje");
                break;
            }
            sent += n;
        }
    }

    close(sock);
    return EXIT_SUCCESS;
}