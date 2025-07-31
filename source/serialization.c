// Grupo SD-01
// Miguel Angel - fc65675
// Gonçalo Moreira - fc56571
// Liliana Valente - fc59846

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "serialization.h"

/* Serializa todas as chaves presentes no array de strings keys para o
* buffer keys_buf, que será alocado dentro da função. A serialização
* deve ser feita de acordo com o seguinte formato:
* | int | string | string | string |
* | nkeys | key1 | key2 | key3 |
* Retorna o tamanho do buffer alocado ou -1 em caso de erro.
*/
int keyArray_to_buffer(char **keys, char **keys_buf) {
    //Verificar se o array de keys é válido
    if (keys == NULL || keys_buf == NULL) {
        return -1;
    }

    //Calcular a nkeys e o tamanho do buffer
    int size = 0;
    int nkeys = 0;
    while (keys[nkeys] != NULL) {
        size += strlen(keys[nkeys]) + 1;
        nkeys++;
    }

    //Alocar espaço para o buffer
    size += sizeof(int);
    *keys_buf = malloc(size);

    //Verificar se o buffer foi alocado com sucesso
    if (*keys_buf == NULL) {
        return -1;
    }

    //Serializar o nkeys
    int nkeys_s = htonl(nkeys);
    memcpy(*keys_buf, &nkeys_s, sizeof(int)); 

    //Serializar as keys
    char *buf_ptr = *keys_buf + sizeof(int);
    for (int i = 0; i < nkeys; i++) {
        size_t key_len = strlen(keys[i]) + 1; 
        memcpy(buf_ptr, keys[i], key_len); 
        buf_ptr += key_len; 
    }

    return size;
}
/* De-serializa a mensagem contida em keys_buf, colocando-a num array de
* strings cujo espaco em memória deve ser reservado. A mensagem contida
* em keys_buf deverá ter o seguinte formato:
* | int | string | string | string |
* | nkeys | key1 | key2 | key3 |
* Retorna o array de strings ou NULL em caso de erro.
*/
char** buffer_to_keyArray(char *keys_buf) {
    //Verificar se keys_buf é válido
    if (keys_buf == NULL) {
        return NULL;
    }

    //Deserializar o nkeys
    int nkeys;
    memcpy(&nkeys, keys_buf, sizeof(int));
    nkeys = ntohl(nkeys);

    //Alocar espaço para o array de keys
    char **keys = malloc((nkeys + 1) * sizeof(char *));

    //Verificar se o array foi alocado com sucesso
    if (keys == NULL) {
        return NULL;
    }

    //Deserializar as keys
    char *buf_ptr = keys_buf + sizeof(int);
    for (int i = 0; i < nkeys; i++) {
        size_t key_len = strlen(buf_ptr) + 1;
        keys[i] = malloc(key_len);
        //Verificar se a key foi alocada com sucesso
        if (keys[i] == NULL) {
            for (int j = 0; j < i; j++) {
                free(keys[j]);
            }
            free(keys);
            return NULL;
        }
        memcpy(keys[i], buf_ptr, key_len);
        buf_ptr += key_len;
    }

    return keys;
}