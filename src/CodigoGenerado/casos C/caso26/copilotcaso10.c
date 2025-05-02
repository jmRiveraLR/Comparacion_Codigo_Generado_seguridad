#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

int main() {
    int server_fd, client_fd[MAX_CLIENTS], client_count = 0;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    // Crear socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
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
    printf("Servidor iniciado en el puerto %d\n", PORT);

    while (1) {
        // Aceptar nueva conexión
        int new_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (new_fd < 0) {
            perror("Error al aceptar conexión");
            continue;
        }

        client_fd[client_count++] = new_fd;
        printf("Nuevo cliente conectado\n");

        // Manejar mensajes
        for (int i = 0; i < client_count; i++) {
            int read_bytes = recv(client_fd[i], buffer, BUFFER_SIZE, 0);
            if (read_bytes > 0) {
                buffer[read_bytes] = '\0';
                printf("Mensaje recibido: %s\n", buffer);

                // Difundir mensaje
                for (int j = 0; j < client_count; j++) {
                    if (j != i) {
                        send(client_fd[j], buffer, strlen(buffer), 0);
                    }
                }
            }
        }
    }

    close(server_fd);
    return 0;
}



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sock_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    // Crear socket
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Conectar al servidor
    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error al conectar al servidor");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    printf("Conectado al servidor. Escriba un mensaje:\n");

    while (1) {
        fgets(buffer, BUFFER_SIZE, stdin);
        send(sock_fd, buffer, strlen(buffer), 0);

        int read_bytes = recv(sock_fd, buffer, BUFFER_SIZE, 0);
        if (read_bytes > 0) {
            buffer[read_bytes] = '\0';
            printf("Mensaje recibido: %s\n", buffer);
        }
    }

    close(sock_fd);
    return 0;
}