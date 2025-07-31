// Grupo SD-01
// Miguel Angel - fc65675
// Gonçalo Moreira - fc56571
// Liliana Valente - fc59846

#include "table.h"
#include "table-private.h"
#include "entry.h"
#include "list.h"

#include "stats.h"

#include "list-private.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int stats_init(struct statistics_t *stats) {
    if (stats == NULL) {
        return -1; // Devolver error si `stats` es NULL
    }

    stats->total_operations = 0;
    stats->total_time_us = 0;
    stats->connected_clients = 0;
    stats->mutex = malloc(sizeof(pthread_mutex_t));
    if (stats->mutex == NULL) {
        return -1;
    }
    if (pthread_mutex_init(stats->mutex, NULL) != 0) {
        return -1; // Devolver error si no se puede inicializar el mutex
    }

    return 0;
}

void stats_destroy(struct statistics_t *stats) {
    if (stats == NULL) {
        return; // No hacer nada si `stats` es NULL
    }
    pthread_mutex_destroy(stats->mutex);
}

/* Função para criar e inicializar uma nova tabela hash, com n
 * linhas (n = módulo da função hash).
 * Retorna a tabela ou NULL em caso de erro.
 */
struct table_t *table_create(int n) {
    // Verificar si el argumento es válido
    if (n <= 0) {
        return NULL;
    }

    // Alocar espacio para la tabla
    struct table_t *t = malloc(sizeof(struct table_t));
    
    // Verificar si la tabla fue bien creada
    if (t == NULL) {
        return NULL;
    }

    // Alocar espacio para las estadísticas
    t->statistics = malloc(sizeof(struct statistics_t));
    if (t->statistics == NULL) {
        free(t);
        return NULL;
    }

    // Inicializar las estadísticas
    if (stats_init(t->statistics) != 0) {
        free(t->statistics);
        free(t);
        return NULL;
    }

    // Alocar espacio para los slots
    t->slots = malloc(sizeof(struct list_t *) * n);

    // Verificar si los slots fueron bien creados
    if (t->slots == NULL) {
        stats_destroy(t->statistics);
        free(t->statistics);
        free(t);
        return NULL;
    }

    // Inicializar la tabla
    t->size = n;
    for (int i = 0; i < n; i++) {
        // Verificar si la lista fue bien creada
        if ((t->slots[i] = list_create()) == NULL) {
            for (int j = 0; j < i; j++) {
                list_destroy(t->slots[j]);
            }
            free(t->slots);
            stats_destroy(t->statistics);
            free(t->statistics);
            free(t);
            return NULL;
        }
    }

    return t;
}

/* Função para adicionar um par chave-valor à tabela. Os dados de entrada
 * desta função deverão ser copiados, ou seja, a função vai criar uma nova
 * entry com *CÓPIAS* da key (string) e dos dados. Se a key já existir na
 * tabela, a função tem de substituir a entry existente na tabela pela
 * nova, fazendo a necessária gestão da memória.
 * Retorna 0 (ok) ou -1 em caso de erro.
 */
int table_put(struct table_t *t, char *key, struct block_t *value){
    //Verificar se os argumentos são válidos
    if (t == NULL || key == NULL || value == NULL) {
        return -1;
    }

    //Criar as cópias da key e dos dados
    char *key_copy = strdup(key);
    struct block_t *value_copy = block_duplicate(value);

    //Verificar se as cópias foram bem criadas
    if (key_copy == NULL || value_copy == NULL) {
        free(key_copy);
        block_destroy(value_copy);
        return -1;
    }

    //Criação da nova entry
    struct entry_t *entry = entry_create(key_copy, value_copy);

    //Verificar se a entry foi bem criada
    if (entry == NULL) {
        free(key_copy);
        block_destroy(value_copy);
        return -1;
    }

    //Calcular o hash da key
    int i = hash(key_copy, t->size);

    struct entry_t *existing_entry = list_get(t->slots[i], key_copy);

    //Verificar se a key já existe na tabela
    if (existing_entry != NULL) {
        //Substituir a entry existente pela nova
        list_remove(t->slots[i], key_copy);
    }

    //Adicionar a entry à lista
    int result = list_add(t->slots[i], entry);

    //Verificar se a entry foi bem adicionada
    if (result == -1) {
        entry_destroy(entry);
        return -1;
    }

    return 0;
}

/* Função que procura na tabela uma entry com a chave key.
 * Retorna uma *CÓPIA* dos dados (estrutura block_t) nessa entry ou
 * NULL se não encontrar a entry ou em caso de erro.
 */
struct block_t *table_get(struct table_t *t, char *key){
    //Verificar se os argumentos são válidos
    if (t == NULL || key == NULL) {
        return NULL;
    }

    //Calcular o hash da key
    int i = hash(key, t->size);

    struct entry_t *entry = list_get(t->slots[i], key);

    //Verificar se a entry foi bem obtida
    if (entry == NULL) {
        return NULL;
    }

    //Criar uma cópia dos dados da entry
    struct block_t *value_copy = block_duplicate(entry->value);

    //Verificar se a cópia foi bem criada
    if (value_copy == NULL) {
        return NULL;
    }

    return value_copy;
}

/* Função que conta o número de entries na tabela passada como argumento.
 * Retorna o tamanho da tabela ou -1 em caso de erro.
 */
int table_size(struct table_t *t){
    //Verificar se a table é válida
    if (t == NULL) {
        return -1;
    }

    //Contar o número de entries
    int size = 0;
    for (int i = 0; i < t->size; i++) {
        size += list_size(t->slots[i]);
    }

    return size;
}

/* Função auxiliar que constrói um array de char* com a cópia de todas as keys na
 * tabela, colocando o último elemento do array com o valor NULL e
 * reservando toda a memória necessária.
 * Retorna o array de strings ou NULL em caso de erro.
 */
char **table_get_keys(struct table_t *t){
    //Verificar se a table é válida
    if (t == NULL) {
        return NULL;
    }

    char **keys = malloc(sizeof(char *) * (table_size(t) + 1));

    //Verificar se o array foi alocado com sucesso
    if (keys == NULL) {
        return NULL;
    }

    //Percorrer a table para obter as keys
    int keys_i = 0;
    for (int i = 0; i < t->size; i++) {
        //Obter as keys do slot
        char **slot_keys = list_get_keys(t->slots[i]);
        //Verificar se as keys foram bem obtidas
        if (slot_keys == NULL) {
            list_free_keys(slot_keys);
            return NULL;
         }

        //Copiar as keys para o array
        int slot_i = 0;
        while (slot_keys[slot_i] != NULL) {
            //Alocar espaço para a key
            keys[keys_i] = malloc(strlen(slot_keys[slot_i]) + 1);
            //Verificar se a key foi bem copiada
            if (keys[keys_i] == NULL) {
                list_free_keys(slot_keys);
                table_free_keys(keys);
                return NULL;
            }

            //Copiar a key
            memcpy(keys[keys_i], slot_keys[slot_i], strlen(slot_keys[slot_i]) + 1);
            keys_i++;
            slot_i++;
        }

        list_free_keys(slot_keys);
    }

    //Colocar o último elemento do array com o valor NULL
    keys[keys_i] = NULL;

    return keys;
}

/* Função auxiliar que liberta a memória ocupada pelo array de keys obtido pela
 * função table_get_keys.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int table_free_keys(char **keys){
    //Verificar se o array de keys é válido
    if (keys == NULL) {
        return -1;
    }

    //Percorrer o array para libertar a memória ocupada pelas keys
    int i = 0;
    while (keys[i] != NULL) {
        free(keys[i]);
        i++;
    }

    //Libertar a memória ocupada pelo array
    free(keys);

    return 0;
}

/* Função que remove da tabela a entry com a chave key, libertando a
 * memória ocupada pela entry.
 * Retorna 0 se encontrou e removeu a entry, 1 se não encontrou a entry,
 * ou -1 em caso de erro.
 */
int table_remove(struct table_t *t, char *key){
    //Verificar se os argumentos são válidos
    if (t == NULL || key == NULL) {
        return -1;
    }

    //Calcular o hash da key
    int i = hash(key, t->size);

    return list_remove(t->slots[i], key);
}

/* Função que elimina uma tabela, libertando *toda* a memória utilizada
 * pela tabela.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int table_destroy(struct table_t *t) {
    // Verificar si la tabla es válida
    if (t == NULL) {
        return -1;
    }

    // Liberar la memoria ocupada por los slots
    for (int i = 0; i < t->size; i++) {
        list_destroy(t->slots[i]);
    }

    // Liberar la memoria ocupada por el array de slots
    free(t->slots);

    // Liberar las estadísticas
    stats_destroy(t->statistics);
    free(t->statistics);

    // Liberar la memoria ocupada por la tabla
    free(t);

    return 0;
}

//Função hash para calcular a posição da key na tabela
int hash(char *key, int size){
    //Verifica se a key é valida
    if (key == NULL || size <= 0) {
        return -1;
    }

    // Calcular o hash usando uma abordagem polinomial
    unsigned long hash = 5381;
    int c;

    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c;
    }

    return hash % size;
}