// Grupo SD-01
// Miguel Angel - fc65675
// Gonçalo Moreira - fc56571
// Liliana Valente - fc59846

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "entry.h"

/* Função que cria uma entry, reservando a memória necessária e
 * inicializando-a com a string e o bloco de dados de entrada.
 * Retorna a nova entry ou NULL em caso de erro.
 */
struct entry_t *entry_create(char *key, struct block_t *value) {
    // Verificar que os argumentos são válidos
    if (key == NULL || value == NULL) {
        return NULL;
    }
    
    // Reservar memória para a nova entrada
    struct entry_t *entry = malloc(sizeof(struct entry_t));
    
    // Verificar se a entrada foi bem alocada
    if (entry == NULL) {
        return NULL;
    }

    // Inicializar a entrada (fazer cópias da chave e valor)
    entry->key = strdup(key);  // Duplicar a chave para garantir independência
    if (entry->key == NULL) {
        free(entry);
        return NULL;
    }

    entry->value = malloc(sizeof(struct block_t));
    if (entry->value == NULL) {
        free(entry->key);
        free(entry);
        return NULL;
    }

    entry->value->datasize = value->datasize;
    entry->value->data = malloc(value->datasize);
    if (entry->value->data == NULL) {
        free(entry->value);
        free(entry->key);
        free(entry);
        return NULL;
    }

    memcpy(entry->value->data, value->data, value->datasize);

    return entry;
}

/* Função que compara duas entries e retorna a ordem das mesmas, sendo esta
 * ordem definida pela ordem das suas chaves.
 * Retorna 0 se as chaves forem iguais, -1 se e1 < e2,
 * 1 se e1 > e2 ou -2 em caso de erro.
 */
int entry_compare(struct entry_t *e1, struct entry_t *e2){

    if(e1 == NULL || e2 == NULL || e1->key == NULL || e2->key == NULL) return -2;
    
    int comparison = strcmp(e1->key, e2->key);

    if(comparison == 0) return 0;
    else if(comparison < 0) return -1;
    else return 1;
}

/* Função que duplica uma entry, reservando a memória necessária para a
 * nova estrutura.
 * Retorna a nova entry ou NULL em caso de erro.
 */
struct entry_t *entry_duplicate(struct entry_t *e){
    //Verificar se a entry é válida
    if (e == NULL || e->key == NULL || e->value == NULL) {
        return NULL;
    }
    
    ////Reservar memória ao duplicate
    struct entry_t *duplicate = malloc(sizeof(struct entry_t));
    duplicate->key = malloc(sizeof(char));

    //Duplicar os valores para o duplicate
    memcpy(duplicate->key, e->key, strlen(e->key)+1);
    duplicate->value = block_duplicate(e->value);

    return duplicate;
}

/* Função que substitui o conteúdo de uma entry, usando a nova chave e
 * o novo valor passados como argumentos, e eliminando a memória ocupada
 * pelos conteúdos antigos da mesma.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int entry_replace(struct entry_t *e, char *new_key, struct block_t *new_value){
    //Verificar se a entry é válida
    if (e == NULL || new_key == NULL || new_value == NULL) {
        return -1;
    }
    
    //Libertar a memória dos valores antigos
    block_destroy(e->value);
    free(e->key);
    
    //Substituir os valores antigos pelos novos
    e->key = strdup(new_key);
    e->value = new_value;
    
    return 0;
}

/* Função que elimina uma entry, libertando a memória por ela ocupada.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int entry_destroy(struct entry_t *e){
    //Verificar se a entry é válida
    if (e == NULL || e->key == NULL || e->value == NULL) {
        return -1;
    }
    
    //Libertar a memória toda da entry
    block_destroy(e->value);
    free(e->key);
    free(e);
    
    return 0;
}