// Grupo SD-01
// Miguel Angel - fc65675
// Gonçalo Moreira - fc56571
// Liliana Valente - fc59846

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>         // Para sleep y close
#include <arpa/inet.h>      // Para inet_addr
#include <netinet/in.h>     // Para sockaddr_in y htons
#include "server_skeleton.h"
#include "table.h"
#include "table-private.h"
#include "stats.h"
#include "client_stub.h"
#include <pthread.h>
#include <zookeeper/zookeeper.h>

// Watcher para cambios en la cadena de servidores
void chain_watcher(zhandle_t *zzh, int type, int state, const char *path, void *context) {
    struct server_skeleton_t *server = (struct server_skeleton_t *)context;

    if (type == ZOO_CHILD_EVENT) {
        struct String_vector children;
        int ret = zoo_get_children(server->zh, "/chain", 1, &children);

        if (ret != ZOK) {
            fprintf(stderr, "Erro ao obter filhos de /chain: %s\n", zerror(ret));
            return;
        }

        qsort(children.data, children.count, sizeof(char *), compare_nodes);

        for (int i = 0; i < children.count; i++) {
            if (strcmp(server->node_path, children.data[i]) == 0) {
                if (i + 1 < children.count) {
                    snprintf(server->next_server_path, 1024, "/chain/%s", children.data[i + 1]);
                } else {
                    server->next_server_path[0] = '\0';
                }
            }
        }

        // Confirmar si el servidor es el TAIL
        if (strcmp(server->node_path, children.data[children.count - 1]) == 0) {
            server->is_tail = 1;
            printf("Este servidor es el TAIL.\n");
        } else {
            server->is_tail = 0;
            printf("Este servidor NO es el TAIL.\n");
        }

        deallocate_string_vector(&children);
    }
}

// Comparar nodos para ordenarlos
int compare_nodes(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

// Watcher para la conexión a ZooKeeper
void connection_watcher(zhandle_t *zzh, int type, int state, const char *path, void *context) {
    struct server_skeleton_t *server = (struct server_skeleton_t *)context;

    if (type == ZOO_SESSION_EVENT) {
        if (state == ZOO_CONNECTED_STATE) {
            server->is_conected = 1;
            printf("Conectado a ZooKeeper.\n");

            // Crear nodo del servidor en ZooKeeper
            char created_path[1024] = "";
            char temp_path[1024];
            snprintf(temp_path, sizeof(temp_path), "/chain/server_");

            char server_info[1024];
            snprintf(server_info, sizeof(server_info), "127.0.0.1:%d", server->port);

            if (ZOK == zoo_create(server->zh, temp_path, server_info, strlen(server_info), &ZOO_OPEN_ACL_UNSAFE,
                                  ZOO_EPHEMERAL | ZOO_SEQUENCE, created_path, sizeof(created_path))) {
                printf("Nodo criado: %s con datos %s\n", created_path, server_info);
                server->node_path = strdup(created_path);
            } else {
                fprintf(stderr, "Erro ao criar nodo no ZooKeeper\n");
                server->is_conected = 0;
                return;
            }
        } else {
            server->is_conected = 0;
            printf("Desconectado do ZooKeeper.\n");
        }
    }
}


void setup_zookeeper(struct server_skeleton_t *server) {
    server->zh = zookeeper_init(server->prev_server_path, connection_watcher, 3000, 0, server, 0);
    if (!server->zh) {
        fprintf(stderr, "Error conectando a ZooKeeper.\n");
        exit(EXIT_FAILURE);
    }
    sleep(3); // Esperar para asegurar la conexión
}


// Replicar datos al sucesor
void replicate_to_successor(struct server_skeleton_t *server, const char *key, const char *value, int value_len) {
    if (server->next_server_path[0] != '\0') {
        printf("Replicando al sucesor: %s, Clave: %s, Valor: %s\n",
               server->next_server_path, key, value ? value : "NULL");

        MessageT msg = MESSAGE_T__INIT;
        msg.opcode = value ? MESSAGE_T__OPCODE__OP_PUT : MESSAGE_T__OPCODE__OP_DEL;
        msg.c_type = value ? MESSAGE_T__C_TYPE__CT_ENTRY : MESSAGE_T__C_TYPE__CT_KEY;

        if (value) {
            msg.entry = malloc(sizeof(EntryT));
            entry_t__init(msg.entry);
            msg.entry->key = strdup(key);
            msg.entry->value.data = (uint8_t *)value;
            msg.entry->value.len = value_len;
        } else {
            msg.key = strdup(key);
        }

        if (send_message_to_successor(server->next_server_path, &msg) != 0) {
            fprintf(stderr, "Error al replicar al sucesor.\n");
        }

        if (value) {
            free(msg.entry->key);
            free(msg.entry);
        } else {
            free(msg.key);
        }
    } else {
        printf("No hay sucesor definido. La replicación no es posible.\n");
    }
}


// Envio de mensaje al nodo sucesor
int send_message_to_successor(const char *successor_path, MessageT *msg) {
    int sockfd;
    struct sockaddr_in server_addr;

    // Crear el socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error al crear el socket");
        return -1;
    }

    // Configurar la dirección del servidor sucesor
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(successor_path)); // Cambia esto según cómo determines el puerto
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Suponiendo localhost, ajusta según sea necesario

    // Conectar al sucesor
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error al conectar con el sucesor");
        close(sockfd);
        return -1;
    }

    // Serializar el mensaje
    int msg_size = message_t__get_packed_size(msg);
    uint8_t *buffer = malloc(msg_size);
    if (buffer == NULL) {
        perror("Error al alocar memoria para el mensaje");
        close(sockfd);
        return -1;
    }
    message_t__pack(msg, buffer);

    // Enviar el tamaño del mensaje
    int net_msg_size = htonl(msg_size);
    if (write(sockfd, &net_msg_size, sizeof(int)) < 0) {
        perror("Error al enviar el tamaño del mensaje");
        free(buffer);
        close(sockfd);
        return -1;
    }

    // Enviar el mensaje
    if (write(sockfd, buffer, msg_size) < 0) {
        perror("Error al enviar el mensaje");
        free(buffer);
        close(sockfd);
        return -1;
    }

    free(buffer);
    close(sockfd);
    return 0;
}


// Función para inicializar el servidor con ZooKeeper
struct server_skeleton_t *server_skeleton_init(int n_lists, const char *zookeeper_host, int port) {
    struct server_skeleton_t *server = malloc(sizeof(struct server_skeleton_t));
    if (server == NULL) {
        perror("Error al crear el servidor");
        return NULL;
    }

    server->table = table_create(n_lists);
    if (server->table == NULL) {
        perror("Error al crear la tabla");
        free(server);
        return NULL;
    }

    server->zh = zookeeper_init(zookeeper_host, connection_watcher, 5000, 0, server, 0);
    if (server->zh == NULL) {
        perror("Error al conectar a ZooKeeper");
        table_destroy(server->table);
        free(server);
        return NULL;
    }
    server->port = port; 
    server->node_path = NULL;
    server->next_server_path = malloc(1024);
    server->prev_server_path = malloc(1024);
    if (server->next_server_path == NULL || server->prev_server_path == NULL) {
        perror("Error al asignar memoria para las rutas");
        zookeeper_close(server->zh);
        table_destroy(server->table);
        free(server->next_server_path);
        free(server->prev_server_path);
        free(server);
        return NULL;
    }

    //Inicialmente todo en valor vacio o 0
    server->next_server_path[0] = '\0'; 
    server->prev_server_path[0] = '\0'; 
    server->is_conected = 0;
    server->is_tail = 0;

    if (pthread_mutex_init(&server->mutex, NULL) != 0) {
        perror("Error al inicializar el mutex");
        free(server->next_server_path);
        free(server->prev_server_path);
        zookeeper_close(server->zh);
        table_destroy(server->table);
        free(server);
        return NULL;
    }

    return server;
}


int server_skeleton_destroy(struct server_skeleton_t *server) {
    if (server == NULL) return -1;

    if (server->zh) {
        zookeeper_close(server->zh);
    }
    if (server->node_path) {
        free(server->node_path);
    }
    if (server->next_server_path) {
        free(server->next_server_path);
    }
    if (server->prev_server_path) {
        free(server->prev_server_path);
    }
    if (server->table) {
        table_destroy(server->table);
    }
    pthread_mutex_destroy(&server->mutex);
    free(server);

    return 0;
}



// Liberar memoria de String_vector
void deallocate_string_vector(struct String_vector *vector) {
    for (int i = 0; i < vector->count; i++) {
        free(vector->data[i]);
    }
    free(vector->data);
}

int invoke(MessageT *msg, struct table_t *table, struct server_skeleton_t *server)
{
    switch (msg->opcode)
    {
    case MESSAGE_T__OPCODE__OP_PUT:
        if (msg->c_type == MESSAGE_T__C_TYPE__CT_ENTRY) {
            struct block_t *block = malloc(sizeof(struct block_t));
            block->datasize = msg->entry->value.len;
            block->data = strdup((const char *)msg->entry->value.data);

            // Operación local
            if (table_put(server->table, msg->entry->key, block) == 0) {
                printf("Clave %s insertada localmente.\n", msg->entry->key);

                // Replicación al sucesor
                if (!server->is_tail) {
                    replicate_to_successor(server, msg->entry->key, (const char *)msg->entry->value.data, msg->entry->value.len);
                }

                // Actualización del mensaje de respuesta
                msg->opcode = MESSAGE_T__OPCODE__OP_PUT + 1;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            } else {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                fprintf(stderr, "Error al insertar clave %s.\n", msg->entry->key);
            }
            free(block->data);
            free(block);
        }
        break;
    case MESSAGE_T__OPCODE__OP_GET:
        if (server->is_tail) {
            if (msg->c_type == MESSAGE_T__C_TYPE__CT_KEY) {
                struct block_t *block = table_get(server->table, msg->key);
                if (block != NULL) {
                    msg->opcode = MESSAGE_T__OPCODE__OP_GET + 1;
                    msg->c_type = MESSAGE_T__C_TYPE__CT_VALUE;
                    msg->value.data = malloc(block->datasize);
                    memcpy(msg->value.data, block->data, block->datasize);
                    msg->value.len = block->datasize;
                    block_destroy(block);
                } else {
                    msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                    msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                }
            }
        } else {
            printf("Operación GET rechazada: este servidor no es el TAIL.\n");
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        }
        break;
    case MESSAGE_T__OPCODE__OP_DEL:
        if (msg->c_type == MESSAGE_T__C_TYPE__CT_KEY) {
            // Operación local
            if (table_remove(server->table, msg->key) == 0) {
                printf("Clave %s eliminada localmente.\n", msg->key);

                // Replicación al sucesor
                if (!server->is_tail) {
                    replicate_to_successor(server, msg->key, NULL, 0);
                }

                // Actualización del mensaje de respuesta
                msg->opcode = MESSAGE_T__OPCODE__OP_DEL + 1;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            } else {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                fprintf(stderr, "Error al eliminar clave %s.\n", msg->key);
            }
        }
        break;
    case MESSAGE_T__OPCODE__OP_SIZE:
        if (server->is_tail) {
            int size = table_size(server->table);
            msg->opcode = MESSAGE_T__OPCODE__OP_SIZE + 1;
            msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
            msg->result = size;
        } else {
            printf("Operación SIZE rechazada: este servidor no es el TAIL.\n");
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        }
        break;
    case MESSAGE_T__OPCODE__OP_GETKEYS:
        if (server->is_tail) { // Procesar solo si es el TAIL
            if (msg->c_type == MESSAGE_T__C_TYPE__CT_NONE)
            {
                if (table_get_keys(table) != NULL)
                {
                    pthread_mutex_lock(table->statistics->mutex);
                    table->statistics->total_operations++;
                    pthread_mutex_unlock(table->statistics->mutex);

                    msg->n_keys = table_size(table);
                    msg->keys = table_get_keys(table);
                    msg->opcode = MESSAGE_T__OPCODE__OP_GETKEYS + 1;
                    msg->c_type = MESSAGE_T__C_TYPE__CT_KEYS;
                }
                else
                {
                    msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                    msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                    fprintf(stderr, "Operación GETKEYS enviada a un nodo que no es el TAIL.\n");
                }
            }
        }
        break;
    case MESSAGE_T__OPCODE__OP_GETTABLE:
        if (server->is_tail) {
            int size = table_size(table);
            if (size > 0) {
                msg->opcode = MESSAGE_T__OPCODE__OP_GETTABLE + 1;
                msg->c_type = MESSAGE_T__C_TYPE__CT_TABLE;

                msg->n_entries = size;
                msg->entries = malloc(sizeof(EntryT *) * size);

                char **keys = table_get_keys(table);
                for (int i = 0; i < size; i++) {
                    msg->entries[i] = malloc(sizeof(EntryT));
                    entry_t__init(msg->entries[i]);
                    msg->entries[i]->key = strdup(keys[i]);

                    struct block_t *block = table_get(table, keys[i]);
                    msg->entries[i]->value.data = (uint8_t *)malloc(block->datasize);
                    memcpy(msg->entries[i]->value.data, block->data, block->datasize);
                    msg->entries[i]->value.len = block->datasize;

                    block_destroy(block);
                }
                table_free_keys(keys);
            } else {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            }
        } else {
            printf("Operación GETTABLE rechazada en un servidor que no es el TAIL.\n");
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        }
        break;

    case MESSAGE_T__OPCODE__OP_STATS:
        if (server->is_tail) { // Procesar solo si es el TAIL
            if (msg->c_type == MESSAGE_T__C_TYPE__CT_NONE) {
                pthread_mutex_lock(&server->table->statistics->mutex);

                printf("Total de operaciones: %d\n", server->table->statistics->total_operations);
                printf("Tiempo total (us): %ld\n", server->table->statistics->total_time_us);
                printf("Clientes conectados: %d\n", server->table->statistics->connected_clients);

                msg->statistics = malloc(sizeof(StatisticsT));
                if (msg->statistics == NULL) {
                    perror("Error al asignar memoria para estadísticas");
                    pthread_mutex_unlock(&server->table->statistics->mutex);
                    msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                    msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                    break;
                }

                statistics_t__init(msg->statistics);
                msg->statistics->total_ops = server->table->statistics->total_operations;
                msg->statistics->total_time = server->table->statistics->total_time_us;
                msg->statistics->clients = server->table->statistics->connected_clients;

                pthread_mutex_unlock(&server->table->statistics->mutex);

                msg->opcode = MESSAGE_T__OPCODE__OP_STATS + 1;
                msg->c_type = MESSAGE_T__C_TYPE__CT_STATS;
            }
        } else {
            printf("Operación STATS rechazada en un servidor que no es el TAIL.\n");
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        }
        break;

    default:
        printf("Operación no soportada: %d\n", msg->opcode);
        msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
        msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        break;
    }
    return 0;
}
