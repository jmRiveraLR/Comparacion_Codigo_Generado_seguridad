#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 1024

void *receive_messages(void *arg) {
    int sock = *(int *)arg;
    char buffer[BUFFER_SIZE];
    int read_size;

    while ((read_size = recv(sock, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[read_size] = '\0';
        printf("%s", buffer);
        fflush(stdout);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <dirección IP> <puerto>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    pthread_t recv_thread;

    // Crear socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));

    // Convertir dirección IP a formato binario
    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
        perror("Dirección inválida / no soportada");
        exit(EXIT_FAILURE);
    }

    // Conectar al servidor
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) {
        perror("Error al conectar");
        exit(EXIT_FAILURE);
    }

    printf("Conectado al servidor. Escribe mensajes (escribe 'salir' para terminar):\n");

    // Crear hilo para recibir mensajes
    if (pthread_create(&recv_thread, NULL, receive_messages, (void *)&sock) < 0) {
        perror("Error al crear el hilo de recepción");
        exit(EXIT_FAILURE);
    }

    // Leer mensajes del usuario y enviarlos al servidor
    while (1) {
        fgets(buffer, BUFFER_SIZE, stdin);

        if (strcmp(buffer, "salir\n") == 0) {
            break;
        }

        send(sock, buffer, strlen(buffer), 0);
    }

    close(sock);
    return 0;
}