// Grupo SD-01
// Miguel Angel - fc65675
// Gonçalo Moreira - fc56571
// Liliana Valente - fc59846


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "server_network.h"
#include "server_skeleton.h"
#include "message-private.h"
#include "table-private.h"

#include <pthread.h>

int server_network_init(short port) {
    int sockfd;
    struct sockaddr_in server;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Erro ao criar socket");
        return -1;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Erro ao fazer bind");
        close(sockfd);
        return -1;
    }

    if (listen(sockfd, 5) < 0) { // Permite até 5 conexões pendentes
        perror("Erro ao executar listen");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

void *handle_client(void *args) {
    if (args == NULL) {
        fprintf(stderr, "Erro ao passar argumentos para a thread secundaria\n");
        return NULL;
    }

    thread_args_t *client_args = (thread_args_t *)args;
    int connsockfd = client_args->connsockfd;
    struct server_skeleton_t *server = client_args->server;

    pthread_mutex_lock(server->table->statistics->mutex);
    server->table->statistics->connected_clients++;
    pthread_mutex_unlock(server->table->statistics->mutex);

    MessageT *msg;
    while ((msg = network_receive(connsockfd)) != NULL) {
        invoke(msg, server->table, server);

        if (msg->opcode == MESSAGE_T__OPCODE__OP_PUT + 1 || msg->opcode == MESSAGE_T__OPCODE__OP_DEL + 1) {
            replicate_to_successor(server, msg->entry->key, (const char *)msg->entry->value.data, msg->entry->value.len);
        }

        network_send(connsockfd, msg);
    }

    pthread_mutex_lock(server->table->statistics->mutex);
    server->table->statistics->connected_clients--;
    pthread_mutex_unlock(server->table->statistics->mutex);

    close(connsockfd);
    free(args);
    pthread_exit(NULL);
}

int network_main_loop(int listening_socket, struct server_skeleton_t *server) {
    if (server == NULL || server->table == NULL) {
        fprintf(stderr, "Servidor ou tabela não inicializados\n");
        return -1;
    }

    while (1) {
        int connsockfd = accept(listening_socket, NULL, NULL);
        if (connsockfd < 0) {
            perror("Erro ao aceitar conexão");
            return -1;
        }

        thread_args_t *args = malloc(sizeof(thread_args_t));
        if (args == NULL) {
            perror("Erro ao alocar memória para argumentos da thread");
            close(connsockfd);
            return -1;
        }

        args->connsockfd = connsockfd;
        args->server = server;
        args->stats = server->table->statistics;

        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, handle_client, (void *)args) != 0) {
            perror("Erro ao criar thread para cliente");
            close(connsockfd);
            free(args);
            return -1;
        }

        pthread_detach(client_thread);
    }

    return -1; // Nunca debería alcanzar este punto
}

MessageT *network_receive(int client_socket) {
    int msg_size;
    if (read_all(client_socket, &msg_size, sizeof(int)) < 0) {
        perror("Erro ao ler o tamanho da mensagem");
        return NULL;
    }
    msg_size = ntohl(msg_size);

    uint8_t *buffer = malloc(msg_size);
    if (buffer == NULL) {
        perror("Erro ao alocar memória para a mensagem");
        return NULL;
    }

    if (read_all(client_socket, buffer, msg_size) < 0) {
        perror("Erro ao ler a mensagem");
        free(buffer);
        return NULL;
    }

    MessageT *msg = message_t__unpack(NULL, msg_size, buffer);
    free(buffer);
    return msg;
}

int network_send(int client_socket, MessageT *msg) {
    int msg_size = message_t__get_packed_size(msg);
    uint8_t *buffer = malloc(msg_size);
    if (buffer == NULL)
        return -1;

    message_t__pack(msg, buffer);

    int net_msg_size = htonl(msg_size);
    if (write_all(client_socket, &net_msg_size, sizeof(int)) < 0 ||
        write_all(client_socket, buffer, msg_size) < 0) {
        free(buffer);
        return -1;
    }

    free(buffer);
    return 0;
}

int server_network_close(int socket) {
    return close(socket);
}
