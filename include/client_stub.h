#ifndef _CLIENT_STUB_H
#define _CLIENT_STUB_H

#include "block.h"
#include "entry.h"
#include <zookeeper/zookeeper.h>

/* Remote table, que deve conter as informações necessárias para comunicar
* com o servidor. A definir pelo grupo em client_stub-private.h */
struct rtable_t;

/* Função para estabelecer uma associação entre o cliente e o servidor,
* em que address_port é uma string no formato <hostname>:<port>.
* Retorna a estrutura rtable preenchida, ou NULL em caso de erro.
*/
struct rtable_t *rtable_connect(char *address_port);

/* Termina a associação entre o cliente e o servidor, fechando a
* ligação com o servidor e libertando toda a memória local. * Retorna 0 se tudo correr bem, ou -1 em caso de erro. */ 
int rtable_disconnect(struct rtable_t *rtable);

/* Função para adicionar uma entrada na tabela.
* Se a key já existe, vai substituir essa entrada pelos novos dados.
* Retorna 0 (OK, em adição/substituição), ou -1 (erro).
*/
int rtable_put(struct rtable_t *rtable, struct entry_t *entry);

/* Retorna a entrada da tabela com chave key, ou NULL caso não exista
* ou se ocorrer algum erro.
*/
struct block_t *rtable_get(struct rtable_t *rtable, char *key);

/* Função para remover um elemento da tabela. Vai libertar
* toda a memoria alocada na respetiva operação rtable_put().
* Retorna 0 (OK), ou -1 (chave não encontrada ou erro).
*/
int rtable_del(struct rtable_t *rtable, char *key);

/* Retorna o número de elementos contidos na tabela ou -1 em caso de erro.
*/
int rtable_size(struct rtable_t *rtable);

/* Retorna um array de char* com a cópia de todas as keys da tabela,
* colocando um último elemento do array a NULL.
* Retorna NULL em caso de erro.
*/
char **rtable_get_keys(struct rtable_t *rtable);

/* Liberta a memória alocada por rtable_get_keys().
*/
void rtable_free_keys(char **keys);

/* Retorna um array de entry_t* com todo o conteúdo da tabela, colocando
* um último elemento do array a NULL. Retorna NULL em caso de erro.
*/
struct entry_t **rtable_get_table(struct rtable_t *rtable);

/* Liberta a memória alocada por rtable_get_table().
*/
void rtable_free_entries(struct entry_t **entries);

/* Obtém as estatísticas do servidor.
*/
struct statistics_t *rtable_stats(struct rtable_t *rtable);

/*
*   Comparador de nodos para enconcontrar cual de los dos es menor
*   Comparacion lexicografica en la que el nodo 'a' es mayor que el nodo 'b'
*   
*   Devuelve: 
*       negativo , si a < b
*       positivo , si a > b
*       0        , si a == b
*/
int compare_nodes(const void *a, const void *b);

/*
*   Funcion que devuelve el identificador del nodo TAIL del 
*   conjunto de servidores que hay
*/
char *get_tail_address(zhandle_t *zh);

/*
*   Funcion que libera la memoria de la estructura de Zookeeper
*/
void deallocate_string_vector(struct String_vector *vector);

#endif
