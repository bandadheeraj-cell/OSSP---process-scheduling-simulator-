#include "../include/pssim_threads.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

pssim_telemetry_t g_telemetry;
static pthread_t g_monitor_thread;
static int g_monitor_running = 0;

static void *pssim_monitor_thread_func(void *arg) {
    (void)arg;
    while (g_monitor_running) {
        pthread_mutex_lock(&g_telemetry.lock);
        g_telemetry.cpu_monitor_ticks += 1.0;
        pthread_mutex_unlock(&g_telemetry.lock);
        sleep(1); /* Telemetry tick every 1 second */
    }
    return NULL;
}

void pssim_init_threads(void) {
    g_telemetry.total_commands_executed = 0;
    g_telemetry.total_processes_scheduled = 0;
    g_telemetry.cpu_monitor_ticks = 0;
    g_telemetry.active_background_jobs = 0;

    pthread_mutex_init(&g_telemetry.lock, NULL);

    g_monitor_running = 1;
    if (pthread_create(&g_monitor_thread, NULL, pssim_monitor_thread_func, NULL) != 0) {
        perror("[PSSIM CONCURRENCY ERROR] Failed to create background monitor thread");
    }
}

void pssim_cleanup_threads(void) {
    if (g_monitor_running) {
        g_monitor_running = 0;
        pthread_join(g_monitor_thread, NULL);
    }
    pthread_mutex_destroy(&g_telemetry.lock);
}

void pssim_telemetry_inc_cmd(void) {
    pthread_mutex_lock(&g_telemetry.lock);
    g_telemetry.total_commands_executed++;
    pthread_mutex_unlock(&g_telemetry.lock);
}

void pssim_telemetry_inc_sched(void) {
    pthread_mutex_lock(&g_telemetry.lock);
    g_telemetry.total_processes_scheduled++;
    pthread_mutex_unlock(&g_telemetry.lock);
}

void pssim_print_telemetry(void) {
    pthread_mutex_lock(&g_telemetry.lock);
    printf("\n%s=== POSIX Concurrency & System Telemetry ===%s\n", PSSIM_COLOR_CYAN, PSSIM_COLOR_RESET);
    printf("Total External Commands Executed: %lu\n", g_telemetry.total_commands_executed);
    printf("Total Scheduled Workloads Executed: %lu\n", g_telemetry.total_processes_scheduled);
    printf("Background Monitor CPU Ticks:      %.0f s\n", g_telemetry.cpu_monitor_ticks);
    printf("Active Background Job Threads:    %d\n", g_telemetry.active_background_jobs);
    pthread_mutex_unlock(&g_telemetry.lock);
}
