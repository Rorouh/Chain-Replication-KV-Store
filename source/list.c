// Grupo SD-01
// Miguel Angel - fc65675
// Gonçalo Moreira - fc56571
// Liliana Valente - fc59846

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "list.h"
#include "list-private.h"

/* Função que cria e inicializa uma nova lista (estrutura list_t a
 * ser definida pelo grupo no ficheiro list-private.h).
 * Retorna a lista ou NULL em caso de erro.
 */
struct list_t *list_create() {
    // Criar uma nova lista
    struct list_t *new_list = malloc(sizeof(struct list_t));
    
    // Verificar se a lista foi criada com sucesso
    if (new_list == NULL) {
        return NULL;
    }

    // Inicializar a nova lista
    new_list->head = NULL;
    new_list->size = 0;
    
    return new_list;
}

/* Função que adiciona à lista a entry passada como argumento.
 * A entry é inserida de forma ordenada, tendo por base a comparação
 * de entries feita pela função entry_compare do módulo entry e
 * considerando que a entry menor deve ficar na cabeça da lista.
 * Se já existir uma entry igual (com a mesma chave), a entry
 * já existente na lista será substituída pela nova entry,
 * sendo libertada a memória ocupada pela entry antiga.
 * Retorna 0 se a entry ainda não existia, 1 se já existia e foi
 * substituída, ou -1 em caso de erro.
 */
int list_add(struct list_t *l, struct entry_t *entry) {
    //Verificar se a lista e a entry são válidas
    if (l == NULL || entry == NULL) {
        return -1;
    }
    
    //Criar um novo node
    struct node_t *new_node = malloc(sizeof(struct node_t));
    
    //Verificar se o node foi criado com sucesso
    if (new_node == NULL) {
        return -1;
    }

    //Inicializar o novo node
    new_node->entry = entry;
    new_node->size = entry->value->datasize;
    new_node->next = NULL;

    //Verificar se a lista está vazia
    if (l->head == NULL) {
        l->head = new_node;
        l->size++;

        return 0;
    }

    struct node_t *current = l->head;

    //Verificar se a nova entry deve ser a primeira (head) da lista
    if (entry_compare(entry, current->entry) == -1) {
        new_node->next = current;
        l->head = new_node;
        l->size++;

        return 0;
    }

    //Percorrer a lista para encontrar a posição correta da nova entry
    while (current->next != NULL) {
        //Verificar em caso de erro
        if (entry_compare(entry, current->entry) == -2 ||
            entry_compare(entry, current->next->entry) == -2) {
            return -1;
        }
        //Verificar se a entry já existe na lista
        else if (entry_compare(entry, current->entry) == 0) {
            entry_destroy(current->entry);
            current->entry = entry;
            return 1;
        }
        //Verificar se a nova entry deve ser inserida entre dois nodes
        else if (entry_compare(entry, current->next->entry) == -1) {
            new_node->next = current->next;
            current->next = new_node;
            l->size++;

            return 0;
        }
		
        current = current->next;
    }

    //Adicionar a nova entry no final da lista caso não exista uma posição correta
    if (entry_compare(entry, current->entry) == 0) {
        current->entry = entry;
        return 1;
    }
    
    current->next = new_node;
    l->size++;

    return 0;

}

/* Função que conta o número de entries na lista passada como argumento.
 * Retorna o tamanho da lista ou -1 em caso de erro.
 */
int list_size(struct list_t *l) {
    if (l == NULL) {
        return -1;
    }

    return l->size;
}

/* Função que obtém da lista a entry com a chave key.
 * Retorna a referência da entry na lista ou NULL se não encontrar a
 * entry ou em caso de erro.
*/
struct entry_t *list_get(struct list_t *l, char *key) {
    // Verificar se a lista e a chave são válidas
    if (l == NULL || key == NULL) {
        return NULL;
    }
    
    struct node_t *current = l->head;

    // Percorrer a lista para encontrar a entry com a chave key
    while (current != NULL) {
        // Caso a chave seja encontrada, retornar a entry
        if (strcmp(current->entry->key, key) == 0) {
            return current->entry;
        }
        current = current->next;
    }

    return NULL;
}

/* Função auxiliar que constrói um array de char* com a cópia de todas as keys na 
 * lista, colocando o último elemento do array com o valor NULL e
 * reservando toda a memória necessária.
 * Retorna o array de strings ou NULL em caso de erro.
 */
char **list_get_keys(struct list_t *l) {
    // Verificar se a lista é válida
    if (l == NULL || l->size <= 0 || l->head == NULL) {
        return NULL;
    }
    // Alocar memória para o array de keys
    char **keys = malloc((l->size + 1) * sizeof(char *));

    // Verificar se o array foi alocado com sucesso
    if (keys == NULL) {
        return NULL;
    }

    struct node_t *current = l->head;

    // Percorrer a lista para copiar as keys para o array
    for (int i = 0; i < l->size; i++) {
        keys[i] = strdup(current->entry->key);
        current = current->next;
    }

    // Colocar o último elemento do array com o valor NULL
    keys[l->size] = NULL;
    return keys;
}

/* Função auxiliar que liberta a memória ocupada pelo array de keys obtido pela 
 * função list_get_keys.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int list_free_keys(char **keys) {
    // Verificar se o array de keys é válido
    if (keys == NULL) {
        return -1;
    }

    // Percorrer o array para libertar a memória ocupada pelas keys
    int i = 0;
    while (keys[i] != NULL) {
        free(keys[i]);
        i++;
    }

    // Libertar a memória ocupada pelo array
    free(keys);

    return 0;
}

/* Função que elimina da lista a entry com a chave key, libertando a
 * memória ocupada pela entry.
 * Retorna 0 se encontrou e removeu a entry, 1 se não encontrou a entry,
 * ou -1 em caso de erro.
 */
int list_remove(struct list_t *l, char *key) {
    // Verificar se a lista e a chave são válidas
    if (l == NULL || key == NULL) {
        return -1;
    }

    struct node_t *current = l->head;

    // Verificar se a entry a remover é a primeira da lista
    if (strcmp(current->entry->key, key) == 0) {
        l->head = current->next;
        entry_destroy(current->entry);
        free(current);
        l->size--;

        return 0;
    }

    // Percorrer a lista para encontrar a entry a remover
    int found = 0;
    while (current->next != NULL && !found) {
        //Caso a chave seja encontrada, remover a entry
        if (strcmp(current->next->entry->key, key) == 0) {
            struct node_t *temp = current->next;
            current->next = current->next->next;
            entry_destroy(temp->entry);
            free(temp);
            l->size--;
            found = 1;

            return 0;
        }
        current = current->next;
    }

    return 1;
}

/* Função que elimina uma lista, libertando *toda* a memória utilizada
 * pela lista (incluindo todas as suas entradas).
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int list_destroy(struct list_t *l) {
    // Verificar se a lista é válida
    if (l == NULL) {
        return -1;
    }

    struct node_t *current = l->head;

    // Percorrer a lista para libertar a memória ocupada pelos nodes
    while (current != NULL) {
        struct node_t *temp = current;
        current = current->next;
        entry_destroy(temp->entry);
        free(temp);
    }

    // Libertar a memória ocupada pela lista
    free(l);

    return 0;
}