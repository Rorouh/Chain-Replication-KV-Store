//client_stub.c
// Grupo SD-01
// Miguel Angel - fc65675
// Gonçalo Moreira - fc56571
// Liliana Valente - fc59846

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "client_stub.h"
#include "client_stub-private.h"
#include "stats.h"
#include "client_network.h"
#include "htmessages.pb-c.h"


struct rtable_t *rtable_connect(char *address_port) {
    // Conexión inicial con ZooKeeper
    zhandle_t *zh = zookeeper_init("localhost:2181", NULL, 3000, 0, 0, 0);
    if (zh == NULL) {
        fprintf(stderr, "Error conectando a ZooKeeper.\n");
        return NULL;
    }

    char *tail_address = get_tail_address(zh);
    if (tail_address == NULL) {
        fprintf(stderr, "Error obteniendo la dirección del TAIL.\n");
        zookeeper_close(zh);
        return NULL;
    }

    printf("Dirección del TAIL obtenida: %s\n", tail_address);

    // Separar dirección IP y puerto
    char *port_str = strchr(tail_address, ':');
    if (port_str == NULL) {
        fprintf(stderr, "Formato inválido de dirección del TAIL: %s\n", tail_address);
        free(tail_address);
        zookeeper_close(zh);
        return NULL;
    }

    *port_str = '\0'; // Separar dirección IP y puerto
    port_str++;       // Apuntar al puerto

    struct rtable_t *rtable = malloc(sizeof(struct rtable_t));
    if (rtable == NULL) {
        perror("Error al asignar memoria para la estructura rtable");
        free(tail_address);
        zookeeper_close(zh);
        return NULL;
    }

    rtable->server_address = strdup(tail_address); // Copiar solo la dirección IP
    rtable->server_port = atoi(port_str);          // Convertir el puerto a entero
    free(tail_address);

    // Conectar al servidor TAIL
    if (network_connect(rtable) != 0) {
        fprintf(stderr, "Erro ao conectar com o servidor TAIL.\n");
        free(rtable->server_address);
        free(rtable);
        zookeeper_close(zh);
        return NULL;
    }

    zookeeper_close(zh);
    printf("Conectado al servidor TAIL: %s:%d\n", rtable->server_address, rtable->server_port);
    return rtable;
}

int rtable_disconnect(struct rtable_t *rtable) {
    if (rtable == NULL) return -1;

    int result = network_close(rtable);
    free(rtable);
    return result;
}

int rtable_put(struct rtable_t *rtable, struct entry_t *entry) {
    if (rtable == NULL || entry == NULL) {
        return -1;
    }

    // Prepara a mensagem de requisição
    MessageT request = MESSAGE_T__INIT;
    request.opcode = MESSAGE_T__OPCODE__OP_PUT; 
    request.c_type = MESSAGE_T__C_TYPE__CT_ENTRY;

    // Crea un EntryT y asegura la correcta inicialización
    EntryT req_entry = ENTRY_T__INIT;
    req_entry.key = strdup(entry->key);
    if (req_entry.key == NULL) {
        return -1;
    }

    req_entry.value.len = entry->value->datasize;
    req_entry.value.data = malloc(entry->value->datasize);
    if (req_entry.value.data == NULL) {
        free(req_entry.key);
        return -1;
    }

    memcpy(req_entry.value.data, entry->value->data, entry->value->datasize);
    request.entry = &req_entry;

    // Envia e recebe a resposta
    MessageT *response = network_send_receive(rtable, &request);
    if (response == NULL || response->opcode != MESSAGE_T__OPCODE__OP_PUT + 1) {
        free(req_entry.value.data);
        free(req_entry.key);
        return -1;
    }

    // Liberar recursos
    free(req_entry.value.data);
    free(req_entry.key);
    message_t__free_unpacked(response, NULL);

    return 0;
}

struct block_t *rtable_get(struct rtable_t *rtable, char *key) {
    if (rtable == NULL || key == NULL) return NULL;

    // Prepara a mensagem de requisição
    MessageT request = MESSAGE_T__INIT;
    request.opcode = MESSAGE_T__OPCODE__OP_GET;
    request.c_type = MESSAGE_T__C_TYPE__CT_KEY;
    request.key = key;

    // Envia e recebe a resposta
    MessageT *response = network_send_receive(rtable, &request);
    if (response == NULL || response->opcode != MESSAGE_T__OPCODE__OP_GET + 1) {
        printf("Error: Respuesta inválida para GET.\n");
        if (response != NULL) message_t__free_unpacked(response, NULL);
        return NULL;
    }

    // Prepara o bloco de dados retornado
    struct block_t *block = malloc(sizeof(struct block_t));
    if (block == NULL) {
        message_t__free_unpacked(response, NULL);
        return NULL;
    }

    block->datasize = response->value.len;
    block->data = malloc(block->datasize);
    if (block->data == NULL) {
        free(block);
        message_t__free_unpacked(response, NULL);
        return NULL;
    }

    memcpy(block->data, response->value.data, block->datasize);
    printf("GET exitoso. Clave: %s, Valor: %s\n", key, (char *)block->data);
    message_t__free_unpacked(response, NULL);

    return block;
}

int rtable_del(struct rtable_t *rtable, char *key) {
    if (rtable == NULL || key == NULL) return -1;

    // Prepara a mensagem de requisição
    MessageT request = MESSAGE_T__INIT;
    request.opcode = MESSAGE_T__OPCODE__OP_DEL;
    request.c_type = MESSAGE_T__C_TYPE__CT_KEY;
    request.key = key;

    // Envia e recebe a resposta
    MessageT *response = network_send_receive(rtable, &request);
    if (response == NULL || response->opcode != MESSAGE_T__OPCODE__OP_DEL + 1) {
        if (response != NULL) {
            message_t__free_unpacked(response, NULL);
        }
        return -1;
    }

    // Liberar recursos
    message_t__free_unpacked(response, NULL);
    return 0;
}

int rtable_size(struct rtable_t *rtable) {
    if (rtable == NULL) return -1;

    // Prepara a mensagem de requisição
    MessageT request = MESSAGE_T__INIT;
    request.opcode = MESSAGE_T__OPCODE__OP_SIZE;
    request.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    // Envia e recebe a resposta
    MessageT *response = network_send_receive(rtable, &request);
    if (response == NULL || response->opcode != MESSAGE_T__OPCODE__OP_SIZE + 1) {
        return -1;
    }

    return response->result;
}

char **rtable_get_keys(struct rtable_t *rtable) {
    if (rtable == NULL) {
        return NULL;
    }

    MessageT request = MESSAGE_T__INIT;
    request.opcode = MESSAGE_T__OPCODE__OP_GETKEYS;
    request.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    MessageT *response = network_send_receive(rtable, &request);
    if (response == NULL || response->opcode != MESSAGE_T__OPCODE__OP_GETKEYS + 1) {
        return NULL;
    }

    // Procesar la respuesta para obtener las claves
    char **keys = malloc(sizeof(char *) * (response->n_keys + 1));
    if (keys == NULL) {
        message_t__free_unpacked(response, NULL);
        return NULL;
    }

    for (int i = 0; i < response->n_keys; i++) {
        keys[i] = strdup(response->keys[i]);
    }
    keys[response->n_keys] = NULL;

    // Liberar la memoria de la respuesta
    message_t__free_unpacked(response, NULL);

    return keys;
}

void rtable_free_keys(char **keys) {
    if (keys == NULL) return;

    for (int i = 0; keys[i] != NULL; i++) {
        free(keys[i]);
    }
    free(keys);
}

struct entry_t **rtable_get_table(struct rtable_t *rtable) {
    if (rtable == NULL) return NULL;

    // Prepara a mensagem de requisição
    MessageT request = MESSAGE_T__INIT;
    request.opcode = MESSAGE_T__OPCODE__OP_GETTABLE;
    request.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    // Envia e recebe a resposta
    MessageT *response = network_send_receive(rtable, &request);
    if (response == NULL || response->opcode != MESSAGE_T__OPCODE__OP_GETTABLE + 1) {
        return NULL;
    }

    // Copia as entries
    struct entry_t **entries = malloc(sizeof(struct entry_t *) * (response->n_entries + 1));
    for (int i = 0; i < response->n_entries; i++) {
        entries[i] = malloc(sizeof(struct entry_t));
        entries[i]->key = strdup(response->entries[i]->key);
        entries[i]->value = malloc(sizeof(struct block_t));
        entries[i]->value->data = malloc(response->entries[i]->value.len);
        entries[i]->value->datasize = response->entries[i]->value.len;
        memcpy(entries[i]->value->data, response->entries[i]->value.data, response->entries[i]->value.len);
    }
    entries[response->n_entries] = NULL;

    return entries;
}

void rtable_free_entries(struct entry_t **entries) {
    if (entries == NULL) return;

    for (int i = 0; entries[i] != NULL; i++) {
        free(entries[i]->key);
        free(entries[i]->value->data);
        free(entries[i]->value);
        free(entries[i]);
    }
    free(entries);
}

struct statistics_t *rtable_stats(struct rtable_t *rtable) {
    if (rtable == NULL) return NULL;

    MessageT request = MESSAGE_T__INIT;
    request.opcode = MESSAGE_T__OPCODE__OP_STATS;
    request.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    MessageT *response = network_send_receive(rtable, &request);
    if (response == NULL || response->opcode != MESSAGE_T__OPCODE__OP_STATS + 1) {
        if (response != NULL) message_t__free_unpacked(response, NULL);
        return NULL;
    }

    struct statistics_t *stats = malloc(sizeof(struct statistics_t));
    if (stats == NULL) {
        perror("Error al asignar memoria para estadísticas");
        message_t__free_unpacked(response, NULL);
        return NULL;
    }

    stats->total_operations = response->statistics->total_ops;
    stats->total_time_us = response->statistics->total_time;
    stats->connected_clients = response->statistics->clients;

    message_t__free_unpacked(response, NULL);
    return stats;
}

char *get_tail_address(zhandle_t *zh) {
    struct String_vector children;
    if (zoo_get_children(zh, "/chain", 0, &children) != ZOK) {
        fprintf(stderr, "Error al obtener los nodos hijos de /chain\n");
        return NULL;
    }

    qsort(children.data, children.count, sizeof(char *), compare_nodes);

    // Último nodo es el TAIL
    char *tail_path = malloc(1024);
    if (tail_path == NULL) {
        deallocate_string_vector(&children);
        return NULL;
    }

    snprintf(tail_path, 1024, "/chain/%s", children.data[children.count - 1]);
    deallocate_string_vector(&children);

    // Obtener la dirección del nodo TAIL
    char tail_address[1024];
    int len = sizeof(tail_address);
    if (zoo_get(zh, tail_path, 0, tail_address, &len, NULL) != ZOK || len <= 0) {
        fprintf(stderr, "Error: Nodo TAIL no contiene datos válidos: %s\n", tail_path);
        free(tail_path);
        return NULL;
    }

    free(tail_path);
    return strdup(tail_address); // Dirección del TAIL
}

//Compara nodos
int compare_nodes(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}


//Liberacion de memoria asociada a el struct de Zookeeper
void deallocate_string_vector(struct String_vector *vector) {
    for (int i = 0; i < vector->count; i++) {
        free(vector->data[i]);
    }
    free(vector->data);
}
