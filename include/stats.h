#ifndef _STATS_H
#define _STATS_H

#include <pthread.h> 
#include "entry.h"

struct statistics_t {
    int total_operations;
    long total_time_us;
    int connected_clients;
    pthread_mutex_t *mutex;  // Mutex para proteger el acceso a las estadísticas
};


int stats_init(struct statistics_t *stats) ;

void stats_destroy(struct statistics_t *stats);

#endif