// Grupo SD-01
// Miguel Angel - fc65675
// Gonçalo Moreira - fc56571
// Liliana Valente - fc59846

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "message-private.h"
#include "client_stub.h"
#include "client_network.h"
#include "server_skeleton.h"
#include "htmessages.pb-c.h"
#include "client_stub-private.h"

/* Esta função deve:
* - Obter o endereço do servidor (struct sockaddr_in) com base na
* informação guardada na estrutura rtable;
* - Estabelecer a ligação com o servidor;
* - Guardar toda a informação necessária (e.g., descritor do socket)
* na estrutura rtable;
* - Retornar 0 (OK) ou -1 (erro).
*/
int network_connect(struct rtable_t *rtable) {
    if (rtable == NULL) return -1;

    int sockfd;
    struct sockaddr_in server;

    // Crear socket TCP
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error al crear socket");
        return -1;
    }

    // Configurar dirección del servidor
    server.sin_family = AF_INET;
    server.sin_port = htons(rtable->server_port);

    if (inet_pton(AF_INET, rtable->server_address, &server.sin_addr) <= 0) {
        perror("Error al convertir la dirección IP");
        close(sockfd);
        return -1;
    }

    // Conectar al servidor
    if (connect(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Error al conectar con el servidor");
        close(sockfd);
        return -1;
    }

    rtable->sockfd = sockfd;
    return 0;
}
/* Esta função deve:
* - Obter o descritor da ligação (socket) da estrutura rtable_t;
* - Serializar a mensagem contida em msg;
* - Enviar a mensagem serializada para o servidor;
* - Esperar a resposta do servidor;
* - De-serializar a mensagem de resposta;
* - Tratar de forma apropriada erros de comunicação;
* - Retornar a mensagem de-serializada ou NULL em caso de erro.
*/
MessageT *network_send_receive(struct rtable_t *rtable, MessageT *msg) {
    if (rtable == NULL || msg == NULL) return NULL;

    int sockfd = rtable->sockfd;
    int msg_size = message_t__get_packed_size(msg);
    uint8_t *buffer = malloc(msg_size);

    if (buffer == NULL) {
        perror("Error al asignar memoria para el mensaje");
        return NULL;
    }

    // Serializar mensaje
    message_t__pack(msg, buffer);

    // Establecer timeout en el socket
    struct timeval timer;
    timer.tv_sec = 5; // 5 segundos de timeout
    timer.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timer, sizeof(timer));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timer, sizeof(timer));

    // Enviar tamaño del mensaje
    int net_msg_size = htonl(msg_size);
    if (write_all(sockfd, &net_msg_size, sizeof(net_msg_size)) < 0) {
        perror("Error al enviar tamaño del mensaje");
        free(buffer);
        return NULL;
    }

    // Enviar mensaje serializado
    if (write_all(sockfd, buffer, msg_size) < 0) {
        perror("Error al enviar mensaje");
        free(buffer);
        return NULL;
    }
    free(buffer);

    // Recibir respuesta del servidor
    if (read_all(sockfd, &net_msg_size, sizeof(net_msg_size)) < 0) {
        perror("Error al leer tamaño de la respuesta");
        return NULL;
    }

    msg_size = ntohl(net_msg_size);
    buffer = malloc(msg_size);

    if (buffer == NULL) {
        perror("Error al asignar memoria para la respuesta");
        return NULL;
    }

    if (read_all(sockfd, buffer, msg_size) < 0) {
        perror("Error al leer mensaje de respuesta");
        free(buffer);
        return NULL;
    }

    // Deserializar respuesta
    MessageT *response = message_t__unpack(NULL, msg_size, buffer);
    free(buffer);

    if (response == NULL) {
        perror("Error al deserializar mensaje de respuesta");
        return NULL;
    }

    return response;
}

/* Fecha a ligação estabelecida por network_connect().
* Retorna 0 (OK) ou -1 (erro).
*/
int network_close(struct rtable_t *rtable) {
    if (rtable == NULL) return -1;
    close(rtable->sockfd);
    return 0;
}