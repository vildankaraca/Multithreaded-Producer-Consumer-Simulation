#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include <time.h>
#include <unistd.h>

// HELPER FUNCTION: Finds a thread in the list by its ID, or creates a new entry if it doesn't exist.
int get_or_create_thread(AppConfig* config, const char* id) {
    // First, check if the thread already exists in our array.
    // We need this because a thread's routing (e.g., P1>A) and its speed (e.g., P1:2) 
    // are written on different lines in the config file.
    for (int i = 0; i < config->thread_count; i++) {
        if (strcmp(config->threads[i].id, id) == 0) {
            return i; // Found it, return its index
        }
    }
    
    // If not found, create a new entry at the end of the list
    int new_idx = config->thread_count;
    strcpy(config->threads[new_idx].id, id);
    config->threads[new_idx].source_buffer = '-'; // Set default/empty value
    config->threads[new_idx].target_buffer = '-'; // Set default/empty value
    config->threads[new_idx].interval_ms = 0;
    config->thread_count++;
    
    return new_idx;
}

int parse_config_file(const char* filename, AppConfig* config) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: File %s not found!\n", filename);
        return 0;
    }

    //    Implicit Dummy Value Handling 
    // We set default buffer sizes to 1 instead of 0. 
    // This way, if a user doesn't declare a buffer in the config (e.g., skips B[n]),
    // the system quietly creates a harmless size-1 dummy buffer behind the scenes 
    // to prevent memory allocation errors (Segmentation Faults).
    config->buffer_a_size = 1; 
    config->buffer_b_size = 1;

    config->deadlock_count = 0;
    config->thread_count = 0; 
    char line[256];

    while (fgets(line, sizeof(line), file)) {
        // Strip out comments (ignore everything after '//' on a line)
        char* comment = strstr(line, "//");
        if (comment != NULL) {
            *comment = '\0';
        }

        // Skip empty lines or lines that only had a comment
        if (strlen(line) <= 1) continue;

        // 1. Parse Basic System Settings (Buffer capacities and total runtime)
        if (sscanf(line, "A[%d]", &config->buffer_a_size) == 1) continue;
        if (sscanf(line, "B[%d]", &config->buffer_b_size) == 1) continue;
        if (sscanf(line, "t:%d", &config->runtime_sec) == 1) continue;

        // Temporary variables for string parsing
        char str1[20], str2[20];
        char char1, char2;
        int time_val;

        // 2. Parse 3-Way Connections (Hybrid/Bridge threads)
        // Example format -> B>C3>A (Thread C3 consumes from B, and produces to A)
        if (sscanf(line, " %c>%[^>]>%c ", &char1, str1, &char2) == 3) {
            int idx = get_or_create_thread(config, str1);
            config->threads[idx].type = PRODUCER_CONSUMER;
            config->threads[idx].source_buffer = char1;
            config->threads[idx].target_buffer = char2;
            continue;
        }

        // 3. Parse 2-Way Connections (Standard Producer or Consumer)
        // Example format -> P1>A (Producer) OR A>C1 (Consumer)
        if (sscanf(line, " %[^>]>%s ", str1, str2) == 2) {
            // If the first part is a single character (A or B), it means buffer gives to thread -> CONSUMER
            if (strlen(str1) == 1 && (str1[0] == 'A' || str1[0] == 'B')) {
                int idx = get_or_create_thread(config, str2);
                config->threads[idx].type = CONSUMER;
                config->threads[idx].source_buffer = str1[0];
            } 
            // If the second part is a single character, it means thread gives to buffer -> PRODUCER
            else if (strlen(str2) == 1 && (str2[0] == 'A' || str2[0] == 'B')) {
                int idx = get_or_create_thread(config, str1);
                config->threads[idx].type = PRODUCER;
                config->threads[idx].target_buffer = str2[0];
            }
            continue;
        }

        // 4. Parse Thread Speeds / Execution Intervals
        // Example format -> P1:2 (Thread P1 operates every 2ms)
        if (sscanf(line, " %[^:]:%d ", str1, &time_val) == 2) {
            // Protection to ensure we don't confuse the global runtime "t:20" with a thread ID
            if (strcmp(str1, "t") != 0) { 
                int idx = get_or_create_thread(config, str1);
                config->threads[idx].interval_ms = time_val;
            }
            continue;
        }
    }

    fclose(file);

    //     Simplified Validation 
    // Since we handle missing buffers implicitly, we only need to ensure 
    // the user provided a total runtime duration (t).
    if (config->runtime_sec == 0) {
        printf("Error: Total runtime (t) is missing or set to 0 in config.\n");
        return 0;
    }

    return 1;
}

void* deadlock_monitor_thread(void* arg) {
    AppConfig* config = (AppConfig*)arg;
    
    printf("[MONITOR] Deadlock Detection System is ACTIVE.\n");

    while (1) {
        // Wake up and check the system state every 2 seconds
        sleep(2); 
        long current_time = time(NULL);
        int deadlock_found = 0;

        for (int i = 0; i < config->thread_count; i++) {
            ThreadConfig* t = &config->threads[i];

            // DEADLOCK CONDITION:
            // If a thread is in a "waiting" state AND hasn't been active for more than 5 seconds
            if (t->is_waiting && (current_time - t->last_active > 5)) {
                printf("\n!!! ALERT: DEADLOCK/BLOCK DETECTED !!!\n");
                printf("-> Thread %s is STUCK waiting for Buffer %c\n", t->id, t->waiting_for);
                printf("-> Time elapsed since last activity: %ld seconds\n", (current_time - t->last_active));
                
                deadlock_found = 1;
                // Increment the global deadlock counter for the final performance report
                config->deadlock_count++;  
            }
        }

        if (!deadlock_found) {
            // Optional: We remain silent if the system is healthy to keep the terminal output clean.
            // You could add a printf("System OK") here if you want periodic heartbeat logs.
        }
    }
    return NULL;
}