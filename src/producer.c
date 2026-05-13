#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>  //zaman takibi için 
#include <sys/time.h> //zaman takibi için
#include "common/utils.h"

void* producer_thread(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    ThreadConfig* cfg = args->config; 

    // Determine which buffer this producer will write to
    SharedBuffer* target = (cfg->target_buffer == 'A') ? args->buf_a : args->buf_b;
    int item_counter = 1;  // The actual data we are producing

    // Reset initial tracking values for metrics
    cfg->items_processed = 0; 
    cfg->total_wait_time_ms = 0;

    struct timeval start_wait, end_wait;

    while (1) {
        // Simulate the time it takes to "produce" or generate an item
        usleep(cfg->interval_ms * 1000); 

        // START WAITING TRACKING
        // Update thread state so we know it's about to wait
        cfg->is_waiting = 1;
        cfg->waiting_for = cfg->target_buffer;

        gettimeofday(&start_wait, NULL);  // Start the wait timer

        // Wait if the buffer is full (sem_wait blocks until there is an empty slot)
        sem_wait(&target->empty); 
        // Lock the buffer before writing to prevent race conditions
        pthread_mutex_lock(&target->mutex);

        // Stop the wait timer and calculate elapsed time in milliseconds
        gettimeofday(&end_wait, NULL);
        double waited_ms = (end_wait.tv_sec - start_wait.tv_sec) * 1000.0 + 
                           (end_wait.tv_usec - start_wait.tv_usec) / 1000.0;
        cfg->total_wait_time_ms += waited_ms;

        // SYNC LOG: Mutex lock acquired
        printf("[SYNC] Thread %s has Lock for Buffer %c\n", cfg->id, cfg->target_buffer);
        
        // UPDATE TRACKING
        // We have the lock, so the thread is no longer waiting
        cfg->is_waiting = 0;
        cfg->last_active = time(NULL);

        // CRITICAL SECTION!
        // Safely add the produced item to the circular buffer
        target->data[target->in] = item_counter;
        target->in = (target->in + 1) % target->size;
        target->count++;
        cfg->items_processed++;  // Increment the number of items successfully processed by this thread
        
        printf("[PRODUCER] Thread %s produced item %d to Buffer %c\n", cfg->id, item_counter, cfg->target_buffer);

        // SYNC LOG: Mutex lock released
        printf("[SYNC] Thread %s released Lock for Buffer %c\n", cfg->id, cfg->target_buffer); 

        // Unlock the buffer so other threads (consumers or other producers) can access it
        pthread_mutex_unlock(&target->mutex);
        // Signal that a new item is available in the buffer (increases the 'full' count)
        sem_post(&target->full);
        
        item_counter++;
    }
    return NULL;
}