// Grupo SD-01
// Miguel Angel - fc65675
// Gonçalo Moreira - fc56571
// Liliana Valente - fc59846

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "block.h"

/* Função que cria um novo bloco de dados block_t e que inicializa 
 * os dados de acordo com os argumentos recebidos, sem necessidade de
 * reservar memória para os dados.	
 * Retorna a nova estrutura ou NULL em caso de erro.
 */
struct block_t *block_create(int size, void *data) {
    // Verificar se os dados são válidos
    if (size <= 0 || data == NULL) {
        return NULL;
    }

    // Reservar memória para o novo bloco
    struct block_t *block = malloc(sizeof(struct block_t));
    
    // Verificar se o bloco foi bem alocado
    if (block == NULL) {
        return NULL;
    }

    block->datasize = size;
    block->data = malloc(size);
    
    // Verificar se a alocação da memória para os dados foi bem sucedida
    if (block->data == NULL) {
        free(block);
        return NULL;
    }

    // Copiar os dados para o novo bloco
    memcpy(block->data, data, size);
    
    return block;
}

/* Função que duplica uma estrutura block_t, reservando a memória
 * necessária para a nova estrutura.
 * Retorna a nova estrutura ou NULL em caso de erro.
 */
struct block_t *block_duplicate(struct block_t *block) {
    if (block == NULL || block->data == NULL) {
        return NULL;
    }

    struct block_t *new_block = malloc(sizeof(struct block_t));
    if (new_block == NULL) {
        return NULL;
    }

    new_block->datasize = block->datasize;

    // Asignar memoria para los datos del nuevo bloque
    new_block->data = malloc(block->datasize);
    if (new_block->data == NULL) {
        free(new_block);
        return NULL;
    }

    // Copiar los datos del bloque original al nuevo bloque
    memcpy(new_block->data, block->data, block->datasize);

    return new_block;
}

/* Função que substitui o conteúdo de um bloco de dados block_t.
 * Deve assegurar que liberta o espaço ocupado pelo conteúdo antigo.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int block_replace(struct block_t *b, int new_size, void *new_data){
    //Verificar se o block é válido
    if (b == NULL || new_size <= 0 || new_data == NULL) {
        return -1;
    }

    //Libertar espaço para os dados
    free(b->data);

    //Substituir os valores do block
    b->datasize = new_size;
    b->data = new_data;

    return 0;
}

/* Função que elimina um bloco de dados, apontado pelo parâmetro b,
 * libertando toda a memória por ele ocupada.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int block_destroy(struct block_t *b) {
    //Verificar se o block é válido
    if (b == NULL || b->data == NULL) {
        return -1;
    }

    //Libertar a memória do block e os seus dados
    free(b->data);
    free(b);

    return 0;
}