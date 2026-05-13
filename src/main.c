#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "common/utils.h"

// HELPER FUNCTION: Initialize the shared buffer
void init_buffer(SharedBuffer* buf, int size) {
    // Allocate dynamic memory for the buffer array based on the given size
    buf->data = (int*)malloc(size * sizeof(int)); 
    buf->size = size;
    buf->count = 0;
    buf->in = 0;
    buf->out = 0;
    
    // Initialize synchronization tools (Mutex and Semaphores)
    pthread_mutex_init(&buf->mutex, NULL);
    sem_init(&buf->empty, 0, size); // Initially, there are 'size' empty slots available
    sem_init(&buf->full, 0, 0);     // Initially, there are 0 full slots (0 items ready to consume)
}

int main(int argc, char *argv[]) {
    AppConfig config;

    // 1. Determine Configuration File
    // If a specific file is provided via terminal (e.g., ./os_project configs/exp1.txt), use it.
    // Otherwise, fall back to the default "config.txt".
    const char* config_filename = (argc > 1) ? argv[1] : "config.txt";

    // 2. Read System Configurations
    if (!parse_config_file(config_filename, &config)) {
        printf("Failed to load configuration from: %s\n", config_filename);
        return 1; // Exit the program if the configuration file cannot be read
    }

    printf("--- System Initialization ---\n");
    // Print the loaded file name to make terminal tracking easier during experiments
    printf("Loaded configuration from: %s\n", config_filename);

    // 3. Initialize Shared Buffers
    SharedBuffer buffer_A, buffer_B;
    init_buffer(&buffer_A, config.buffer_a_size);
    init_buffer(&buffer_B, config.buffer_b_size);
    printf("Buffers A and B initialized successfully.\n");

    // Arrays to store thread IDs and the arguments we will pass to them
    pthread_t thread_ids[MAX_THREADS];
    ThreadArgs args_array[MAX_THREADS];

    printf("Starting %d threads...\n\n", config.thread_count);

    // 4. Create Worker Threads (Producers & Consumers)
    for (int i = 0; i < config.thread_count; i++) {
        // Pass the memory address (&) of the specific thread config to avoid copying the whole struct
        args_array[i].config = &config.threads[i]; 
        args_array[i].buf_a = &buffer_A;
        args_array[i].buf_b = &buffer_B;

        // Reset initial tracking states for the deadlock monitor
        config.threads[i].is_waiting = 0;
        config.threads[i].last_active = time(NULL);

        // Route the thread to the correct function based on its type
        if (config.threads[i].type == PRODUCER) {
            pthread_create(&thread_ids[i], NULL, producer_thread, &args_array[i]);
        } else {
            pthread_create(&thread_ids[i], NULL, consumer_thread, &args_array[i]);
        }
    }

    // 5. Start the Inspector (Deadlock Monitor) Thread
    pthread_t monitor_id;
    pthread_create(&monitor_id, NULL, deadlock_monitor_thread, &config);
    // Run the monitor in the background (detached mode).
    // It doesn't need to be joined later; it will run independently
    pthread_detach(monitor_id);

    // 6. System Timer (Main thread sleeps while worker threads do their jobs)
    printf("\n[SYSTEM] Running for %d seconds...\n-----------------------------------\n", config.runtime_sec);
    
    // Suspend execution of the main thread for the specified runtime duration (t:60)
    sleep(config.runtime_sec);

    // 7. Shutdown Sequence
    printf("\n---------------------------------------------\n[SYSTEM] Time is up! Shutting down all operations.\n");

    // Output the final performance and metrics report
    printf("\n============= PERFORMANCE METRICS =============\n");
    printf("Total Deadlocks Detected: %d\n", config.deadlock_count);
    printf("-------------------------------------------------\n");
    printf("%-10s | %-15s | %-20s | %-32s\n", "THREAD ID", "THROUGHPUT", "BLOCKING TIME (ms)", "AVG WAIT (total wait/item) (ms)");
    printf("-------------------------------------------------\n");

    for (int i = 0; i < config.thread_count; i++) {
        ThreadConfig* t = &config.threads[i];
        // Calculate average wait time per item (prevent division by zero)
        double avg_wait = (t->items_processed > 0) ? (t->total_wait_time_ms / t->items_processed) : 0.0;
        
        printf("%-10s | %-15d | %-20.2f | %-32.2f\n", 
               t->id, 
               t->items_processed, 
               t->total_wait_time_ms, 
               avg_wait);
    }
    printf("===============================================\n\n");
    
    // Terminate the entire process immediately (this safely kills all active worker threads)
    exit(0); 
}