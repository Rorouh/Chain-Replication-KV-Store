#ifndef _SERVER_SKELETON_H
#define _SERVER_SKELETON_H

#include "table.h"
#include "htmessages.pb-c.h"
#include <zookeeper/zookeeper.h>

/* Estrutura que define o skeleton do servidor */
struct server_skeleton_t {
    struct table_t *table;
    zhandle_t *zh; // Conexão ao ZooKeeper
    char *node_path; // Identificador do /node no ZooKeeper
    char *next_server_path; // Identificador do /node do próximo servidor na cadeia
    int is_conected; // Flag que indica se o servidor está conectado ao ZooKeeper
    int is_tail;                     // Flag para indicar si este servidor es el TAIL
    char *prev_server_path;          // Ruta del servidor anterior en la cadena
    pthread_mutex_t mutex;           // Mutex para proteger datos compartidos
    int port;
};

/* Inicia o skeleton da tabela. 
 * O main() do servidor deve chamar esta função antes de poder usar a 
 * função invoke(). O parâmetro n_lists define o número de listas a
 * serem usadas pela tabela mantida no servidor.
 * O parâmetro zookeeper_host define o endereço do servidor ZooKeeper.
 * Retorna a estrutura do skeleton criada ou NULL em caso de erro. 
 */ 
struct server_skeleton_t *server_skeleton_init(int n_lists, const char *zookeeper_host, int port);

/* Liberta toda a memória ocupada pela tabela e todos os recursos
 * e outros recursos usados pelo skeleton.
 * Retorna 0 (OK) ou -1 em caso de erro. 
 */
int server_skeleton_destroy(struct server_skeleton_t *skeleton);

/* Executa na tabela table a operação indicada pelo opcode contido em msg
 * e utiliza a mesma estrutura MessageT para devolver o resultado. 
 * Retorna 0 (OK) ou -1 em caso de erro. 
 */ 
int invoke(MessageT *msg, struct table_t *table, struct server_skeleton_t *server);

/**
 * Configura la conexión inicial con ZooKeeper.
 */
void setup_zookeeper(struct server_skeleton_t *server);

/**
 * Watcher para manejar eventos de conexión a ZooKeeper.
 * @param zzh Handle de ZooKeeper.
 * @param type Tipo de evento.
 * @param state Estado de conexión.
 * @param path Ruta afectada.
 * @param context Contexto del watcher.
 */
void connection_watcher(zhandle_t *zzh, int type, int state, const char *path, void *context);

/**
 * Watcher para manejar cambios en la cadena de servidores.
 * @param zzh Handle de ZooKeeper.
 * @param type Tipo de evento.
 * @param state Estado de conexión.
 * @param path Ruta afectada.
 * @param context Contexto del watcher.
 */
void chain_watcher(zhandle_t *zzh, int type, int state, const char *path, void *context);

/**
 * Replica datos al sucesor en la cadena de replicación.
 * @param server Estructura del servidor.
 * @param data Datos a replicar.
 */
void replicate_to_successor(struct server_skeleton_t *server, const char *key, const char *value, int value_len);

/**
 * Compara dos nodos para ordenarlos alfabéticamente.
 * @param a Nodo A.
 * @param b Nodo B.
 * @return Resultado de la comparación.
 */
int compare_nodes(const void *a, const void *b);

/**
 * Libera los recursos de un vector de cadenas (String_vector).
 * @param vector Vector a liberar.
 */
void deallocate_string_vector(struct String_vector *vector);

/*
*   Envia el mensaje de sucesion de escritura al siguiente nodo
*   De la estructura de Zookeeper
*/
int send_message_to_successor(const char *successor_path, MessageT *msg);
#endif
