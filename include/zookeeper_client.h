// este código que criei parece-me irrelevante, por agora
#ifndef ZOOKEEPER_CLIENT_H
#define ZOOKEEPER_CLIENT_H

#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "table.h"
#include "client_stub-private.h"
// eh necessario ter o zookeeper instalado
#include <zookeeper/zookeeper.h> 

#define ZDATALEN 1024 * 1024

static zhandle_t *zh;

zh = zookeeper_init(host_port, connection_watcher, 2000, 0, NULL, 0);
// Código da aplicação
void connection_watcher(zhandle_t *zzh, int type, int state, const char *path, void* context);

zookeeper_close(zh);

#endif