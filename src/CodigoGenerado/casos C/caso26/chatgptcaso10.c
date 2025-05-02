#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 12345
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

int main() {
    int listener, newfd, fdmax, i, j, nbytes;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr, client_addr;
    socklen_t addrlen;
    fd_set master_set, read_fds;

    // Crear socket TCP
    if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Configurar dirección del servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    memset(&(server_addr.sin_zero), '\0', 8);

    // Asociar socket
    if (bind(listener, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // Escuchar conexiones
    if (listen(listener, MAX_CLIENTS) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Inicializar conjuntos de descriptores
    FD_ZERO(&master_set);
    FD_ZERO(&read_fds);
    FD_SET(listener, &master_set);
    fdmax = listener;

    printf("Servidor listo en el puerto %d...\n", PORT);

    // Bucle principal
    for (;;) {
        read_fds = master_set;
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        for (i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == listener) {
                    // Nueva conexión
                    addrlen = sizeof(client_addr);
                    if ((newfd = accept(listener, (struct sockaddr *)&client_addr, &addrlen)) == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master_set);
                        if (newfd > fdmax) fdmax = newfd;
                        printf("Nueva conexión desde %s en socket %d\n",
                               inet_ntoa(client_addr.sin_addr), newfd);
                    }
                } else {
                    // Datos desde un cliente
                    if ((nbytes = recv(i, buffer, sizeof(buffer), 0)) <= 0) {
                        if (nbytes == 0) {
                            printf("Socket %d desconectado\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i);
                        FD_CLR(i, &master_set);
                    } else {
                        // Difundir mensaje
                        buffer[nbytes] = '\0';
                        printf("Mensaje de %d: %s", i, buffer);
                        for (j = 0; j <= fdmax; j++) {
                            if (FD_ISSET(j, &master_set)) {
                                if (j != listener && j != i) {
                                    if (send(j, buffer, nbytes, 0) == -1) {
                                        perror("send");
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return 0;
}
