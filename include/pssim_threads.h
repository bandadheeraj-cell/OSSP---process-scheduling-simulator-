#ifndef PSSIM_THREADS_H
#define PSSIM_THREADS_H

#include "pssim_core.h"

typedef struct {
    unsigned long total_commands_executed;
    unsigned long total_processes_scheduled;
    double cpu_monitor_ticks;
    int active_background_jobs;
    pthread_mutex_t lock;
} pssim_telemetry_t;

extern pssim_telemetry_t g_telemetry;

void pssim_init_threads(void);
void pssim_cleanup_threads(void);
void pssim_telemetry_inc_cmd(void);
void pssim_telemetry_inc_sched(void);
void pssim_print_telemetry(void);

#endif /* PSSIM_THREADS_H */
