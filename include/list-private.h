#ifndef _LIST_PRIVATE_H
#define _LIST_PRIVATE_H /* Módulo LIST_PRIVATE */

#include "entry.h"

struct node_t {
    struct entry_t *entry;
    int size;
    struct node_t *next;
};

struct list_t {
    struct node_t *head;
    int size;
};

#endif