// Grupo SD-01
// Miguel Angel - fc65675
// Gonçalo Moreira - fc56571
// Liliana Valente - fc59846
#include <stdio.h>
#include <stdlib.h>
#include "server_network.h"
#include "server_skeleton.h"

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Uso: %s <porto_servidor> <n_listas> <zookeeper_host>\n", argv[0]);
        return -1;
    }

    // Convertir argumentos a los tipos necesarios
    short port = (short)atoi(argv[1]);
    int n_lists = atoi(argv[2]);
    const char *zookeeper_host = argv[3];

    // Iniciar el skeleton del servidor
    struct server_skeleton_t *skeleton = server_skeleton_init(n_lists, zookeeper_host,port);
    if (skeleton == NULL) {
        fprintf(stderr, "Erro ao iniciar o skeleton do servidor.\n");
        return -1;
    }

    // Iniciar el socket del servidor
    int server_socket = server_network_init(port);
    if (server_socket < 0) {
        fprintf(stderr, "Erro ao iniciar o socket do servidor.\n");
        server_skeleton_destroy(skeleton);
        return -1;
    }

    // Bucle principal de recepción de conexiones
    printf("Servidor conectado.\n");
    if (network_main_loop(server_socket, skeleton) < 0) { // Pasar el servidor completo
        fprintf(stderr, "Erro na execução do loop principal de rede.\n");
    }

    // Cerrar el socket y liberar el skeleton
    server_network_close(server_socket);
    server_skeleton_destroy(skeleton);

    return 0;
}
