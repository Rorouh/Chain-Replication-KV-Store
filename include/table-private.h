#ifndef _TABLE_PRIVATE_H
#define _TABLE_PRIVATE_H /* Módulo TABLE_PRIVATE */

#include "list.h"
#include "stats.h"
#include <string.h>

/* Esta estrutura define o par {chave, valor} para a tabela
 */
struct table_t {
    int size;
    struct list_t **slots;
    struct statistics_t *statistics;
};

//Função hash para calcular a posição da key na tabela
int hash(char *key, int size);

#endif
