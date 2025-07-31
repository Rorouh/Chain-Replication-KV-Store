//client_hastable.c
// Grupo SD-01
// Miguel Angel - fc65675
// Gonçalo Moreira - fc56571
// Liliana Valente - fc59846

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "client_stub.h"
#include "stats.h"

int main(int argc, char *argv[])
{
    // verificar n de args
    if (argc != 2)
    {
        printf("Uso: %s <hostname:port>\n", argv[0]);
        return -1;
    }
    
    // verificar se endereco e valido
    struct rtable_t *rtable = rtable_connect(argv[1]);
    if (rtable == NULL)
    {
        fprintf(stderr, "Erro ao conectar-se ao servidor.\n");
        return -1;
    }

    char str[20];
    while (1)
    {
        printf("Deseja inserir algum comando?\n");
        fgets(str, 20, stdin);
        str[strcspn(str, "\n")] = 0; // Elimina el '\n' del final que enviamos

        char *token = strtok(str, " ");

        // put
        if (token != NULL && strcmp(token, "put") == 0)
        {
            char *key = strtok(NULL, " ");
            char *value_str = strtok(NULL, " ");
            if (key != NULL && value_str != NULL)
            {
                // Crear un bloque para el valor
                struct block_t *value = block_create(strlen(value_str) + 1, value_str);
                if (value == NULL)
                {
                    printf("Erro ao criar o bloco de dados.\n");
                    continue;
                }

                if (rtable_put(rtable, entry_create(key, value)) != 0)
                    printf("Erro ao inserir na tabela.\n");
                else
                    printf("Elemento inserido com sucesso.\n");
                    
                // Liberar la memoria de la `entry` después de usarla
                entry_destroy(entry_create(key, value));

            }
            else
            {
                printf("Comando 'put' inválido. Use: put <key> <value>\n");
            }
        }

        // Comando 'GET'
        else if (token != NULL && strcmp(token, "get") == 0)
        {
            char *key = strtok(NULL, " ");
            if (key != NULL)
            {
                struct block_t *value = rtable_get(rtable, key);
                if (value == NULL)
                    printf("Erro: chave não encontrada.\n");
                else
                {
                    printf("Valor encontrado: %s\n", (char *)value->data);
                    free(value->data);
                    free(value);
                }
            }
            else
            {
                printf("Comando 'get' inválido. Use: get <key>\n");
            }
        }
        // Comando 'DELETE'
        else if (token != NULL && strcmp(token, "del") == 0)
        {
            char *key = strtok(NULL, " ");
            if (key != NULL)
            {
                if (rtable_del(rtable, key) != 0) 
                    printf("Erro ao deletar.\n");
                else
                    printf("Elemento deletado com sucesso.\n");
            }
            else
            {
                printf("Comando 'del' inválido. Use: del <key>\n");
            }
        }

        // Comando 'SIZE'
        else if (token != NULL && strcmp(token, "size") == 0)
        {
            int size = rtable_size(rtable);
            if (size == -1) 
                printf("Erro ao obter o tamanho da tabela.\n");
            else
                printf("Tamanho atual da tabela: %d\n", size);
        }
        // Comando 'GETKEYS'
        else if (token != NULL && strcmp(token, "getkeys") == 0)
        {
            char **keys = rtable_get_keys(rtable);
            if (keys == NULL)
            {
                printf("Erro ao obter as chaves.\n");
            }
            else
            {
                printf("Chaves na tabela:\n");
                for (int i = 0; keys[i] != NULL; i++)
                {
                    printf("- %s\n", keys[i]);
                    free(keys[i]);
                }
                free(keys);
            }
        }
        // Comando 'GETTABLE'
        else if (token != NULL && strcmp(token, "gettable") == 0)
        {
            struct entry_t **entries = rtable_get_table(rtable);
            if (entries == NULL)
            {
                printf("Erro ao obter a tabela.\n");
            }
            else
            {
                printf("Entradas na tabela:\n");
                for (int i = 0; entries[i] != NULL; i++)
                {
                    printf("- Chave: %s, Valor: %s\n", entries[i]->key, (char *)entries[i]->value->data);
                    entry_destroy(entries[i]);
                }
                free(entries);
            }
        }
        
        // Comando 'STATS'
        else if (token != NULL && strcmp(token, "stats") == 0)
        {
            struct statistics_t *stats = rtable_stats(rtable);
            if (stats == NULL)
            {
                printf("Erro ao obter estatísticas.\n");
            }
            else
            {
                printf("Estatísticas do servidor:\n");
                printf("- Operações totais: %d\n", stats->total_operations);
                printf("- Tempo total (us): %ld\n", stats->total_time_us);
                printf("- Clientes conectados: %d\n", stats->connected_clients);
                free(stats);
            }
        }
        // quit
        else if (token != NULL && strcmp(token, "quit") == 0)
        {
            rtable_disconnect(rtable);
            rtable = NULL;
            break;  // Salir del bucle para terminar de forma limpia
        }
        // Comando inválido
        else
        {
            printf("Comando inserido invalido!\n\n");
        }
    }

    return 0;
}
