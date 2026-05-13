#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <sys/time.h>
#include "common/utils.h"

void* consumer_thread(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    ThreadConfig* cfg = args->config;

    // Determine which buffer is our source (where we consume from)
    SharedBuffer* source = (cfg->source_buffer == 'A') ? args->buf_a : args->buf_b;
    SharedBuffer* target = NULL;

    // If this thread is a hybrid (PRODUCER_CONSUMER), set up the target buffer
    if (cfg->type == PRODUCER_CONSUMER) {
        target = (cfg->target_buffer == 'A') ? args->buf_a : args->buf_b;
    }

    cfg->items_processed = 0;
    cfg->total_wait_time_ms = 0;

    struct timeval start_wait, end_wait;

    while (1) {
        // WAITING FOR SOURCE BUFFER
        // Update thread state for logging
        cfg->is_waiting = 1;
        cfg->waiting_for = cfg->source_buffer;

        // Start wait timer
        gettimeofday(&start_wait, NULL);

        // Wait if the buffer is empty (sem_wait blocks until there's an item)
        sem_wait(&source->full);
        pthread_mutex_lock(&source->mutex);

        // Stop wait timer and calculate elapsed time in milliseconds
        gettimeofday(&end_wait, NULL);
        double waited_ms = (end_wait.tv_sec - start_wait.tv_sec) * 1000.0 + 
                           (end_wait.tv_usec - start_wait.tv_usec) / 1000.0;
        cfg->total_wait_time_ms += waited_ms;

        // SYNC LOG: Mutex lock acquired
        printf("[SYNC] Thread %s has Lock for Buffer %c\n", cfg->id, cfg->source_buffer);
        
        // PROCESSING ITEM
        cfg->is_waiting = 0;
        cfg->last_active = time(NULL);

        // Read the item and update buffer circular array index
        int item = source->data[source->out];
        source->out = (source->out + 1) % source->size;
        source->count--;

        // Track how many items we processed (useful for throughput metrics)
        cfg->items_processed++;
        
        printf("[CONSUMER] Thread %s consumed item %d from Buffer %c\n", cfg->id, item, cfg->source_buffer);

        // SYNC LOG: Mutex lock released
        printf("[SYNC] Thread %s released Lock for Buffer %c\n", cfg->id, cfg->source_buffer);

        // Unlock the buffer and signal that there is now an empty slot available
        pthread_mutex_unlock(&source->mutex);
        sem_post(&source->empty);

        usleep(cfg->interval_ms * 1000);  //hesaplama simülasyonu


        // If this thread acts as a bridge (PRODUCER_CONSUMER), move the item to the target buffer
        if (cfg->type == PRODUCER_CONSUMER && target != NULL) {

            // Same tracking logic for the target buffer
            cfg->is_waiting = 1;
            cfg->waiting_for = cfg->target_buffer;

            gettimeofday(&start_wait, NULL);

            // Wait if the target buffer is full
            sem_wait(&target->empty);
            // Lock the target buffer before writing
            pthread_mutex_lock(&target->mutex);

            gettimeofday(&end_wait, NULL);
            double target_waited_ms = (end_wait.tv_sec - start_wait.tv_sec) * 1000.0 + 
                                      (end_wait.tv_usec - start_wait.tv_usec) / 1000.0;
            cfg->total_wait_time_ms += target_waited_ms;

            // SYNC LOG: Mutex lock acquired for target buffer
            printf("[SYNC] Thread %s has Lock for Buffer %c\n", cfg->id, cfg->target_buffer);
            
            cfg->is_waiting = 0;
            cfg->last_active = time(NULL);

            // Write the item and update target buffer indexes
            target->data[target->in] = item;
            target->in = (target->in + 1) % target->size;
            target->count++;
            
            printf("[PROD_CONS] Thread %s moved item %d to Buffer %c\n", cfg->id, item, cfg->target_buffer);

            // SYNC LOG: Mutex lock released for target buffer
            printf("[SYNC] Thread %s released Lock for Buffer %c\n", cfg->id, cfg->target_buffer);
            
            // Unlock the target buffer and signal that there is a new item available
            pthread_mutex_unlock(&target->mutex);
            sem_post(&target->full);
        }
    }
    return NULL;
}