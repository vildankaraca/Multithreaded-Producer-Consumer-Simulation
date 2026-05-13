#ifndef UTILS_H
#define UTILS_H

#include <pthread.h>
#include <semaphore.h>

// Define the maximum number of threads allowed in the system
#define MAX_THREADS 20

// Enum to distinguish the roles of our threads
typedef enum {
    PRODUCER,
    CONSUMER,
    PRODUCER_CONSUMER   // Acts as a bridge: consumes from one buffer, produces to another
} ThreadType;

//Holds all properties and live metrics for a single thread
typedef struct {
    char id[10];             // e.g., "P1", "C2"    
    ThreadType type;       
    char source_buffer;      // Where it reads from (if it's a consumer)
    char target_buffer;      // Where it writes to (if it's a producer)
    int interval_ms;         // Execution delay (speed/interval)
    
    // Fields added specifically for the Deadlock Monitor
    int is_waiting;        // 1 if waiting for a lock/resource, 0 if actively processing
    char waiting_for;      // Which buffer is it stuck waiting for? ('A' or 'B')
    long last_active;      // Timestamp of the last successful operation (in seconds

    // Performance metrics
    int items_processed;       // Total items handled (used to calculate throughput
    double total_wait_time_ms; // Total time spent blocked/waiting for resources
} ThreadConfig;

// GLOBAL APPLICATION CONFIGURATION
// Holds the parsed data from config.txt and overall system state
typedef struct {
    int buffer_a_size;   // Capacity of Buffer A
    int buffer_b_size;   // Capacity of Buffer B
    int runtime_sec;     // Total time the system will run before shutting down
    int thread_count;    // How many threads are currently registered
    ThreadConfig threads[MAX_THREADS];   // Array holding configurations for all threads

    int deadlock_count;   // Total number of deadlocks detected by the monitor
} AppConfig;

// CIRCULAR QUEUE (SHARED BUFFER) STRUCTURE
// The shared resource where producers put items and consumers take them
typedef struct {
    int* data;        // Dynamic array holding the actual items
    int size;         // Maximum capacity
    int count;        // Current number of items in the buffer
    int in;           // Index for the next write operation (Producer)       
    int out;          // Index for the next read operation (Consumer)
    
    // Synchronization tools
    pthread_mutex_t mutex;  // Lock to ensure only one thread modifies the buffer at a time
    sem_t empty;            // Tracks empty slots (blocks Producer if buffer is full)
    sem_t full;             // Tracks filled slots (blocks Consumer if buffer is empty)
} SharedBuffer;

// Function Signatures
int parse_config_file(const char* filename, AppConfig* config);

// THREAD ARGUMENTS
// Passed to each thread upon creation so it knows its config and shared buffers
typedef struct {
    // Pointer to the thread's specific config in the global AppConfig array.
    // We use a pointer (*) so updates to metrics (like items_processed) reflect globally.
    ThreadConfig* config;    
    SharedBuffer* buf_a;
    SharedBuffer* buf_b;
} ThreadArgs;

// Core Thread Function Signatures
void* producer_thread(void* arg);
void* consumer_thread(void* arg);
void* deadlock_monitor_thread(void* arg); // Signature for our Inspector thread

#endif