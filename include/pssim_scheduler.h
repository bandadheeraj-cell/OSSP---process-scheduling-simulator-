#ifndef PSSIM_SCHEDULER_H
#define PSSIM_SCHEDULER_H

#include "pssim_core.h"
#include "pssim_parser.h"

#define PSSIM_MAX_SIM_PROCESSES 128
#define PSSIM_MAX_GANTT_ENTRIES 1024

typedef enum {
    PSSIM_PROC_NEW = 0,
    PSSIM_PROC_READY,
    PSSIM_PROC_RUNNING,
    PSSIM_PROC_WAITING,
    PSSIM_PROC_TERMINATED
} pssim_proc_state_t;

typedef enum {
    PSSIM_SCHED_FCFS = 0,
    PSSIM_SCHED_SJF,
    PSSIM_SCHED_SRTF,
    PSSIM_SCHED_PRIORITY,
    PSSIM_SCHED_PRIORITY_P,
    PSSIM_SCHED_RR,
    PSSIM_SCHED_MLFQ
} pssim_sched_algo_t;

/* Process Control Block (PCB) for Simulator */
typedef struct {
    int pid;
    char name[32];
    int arrival_time;
    int burst_time;
    int remaining_time;
    int priority;
    int start_time;
    int completion_time;
    int turnaround_time;
    int waiting_time;
    int response_time;
    pssim_proc_state_t state;
    
    /* MLFQ fields */
    int queue_level;
    int time_in_current_queue;
    int last_executed_tick;
} pssim_pcb_t;

/* Gantt Chart Entry */
typedef struct {
    int pid;
    char name[32];
    int start_time;
    int end_time;
} pssim_gantt_entry_t;

/* Simulation Metrics */
typedef struct {
    double avg_waiting_time;
    double avg_turnaround_time;
    double avg_response_time;
    double cpu_utilization;
    double throughput;
    int total_execution_ticks;
    int busy_ticks;
    int total_context_switches;
    int process_count;
    char algo_name[64];
} pssim_metrics_t;

/* Global Scheduler Control Structure */
typedef struct {
    pssim_pcb_t process_queue[PSSIM_MAX_SIM_PROCESSES];
    int process_count;
    
    pssim_pcb_t run_queue[PSSIM_MAX_SIM_PROCESSES];
    int run_count;

    pssim_gantt_entry_t gantt[PSSIM_MAX_GANTT_ENTRIES];
    int gantt_count;

    pssim_metrics_t last_metrics;
    int is_simulated;
} pssim_scheduler_t;

extern pssim_scheduler_t g_scheduler;

void pssim_sched_init(void);
void pssim_sched_reset(void);
int pssim_sched_add_process(const char *name, int arrival, int burst, int priority);
void pssim_sched_list(void);
void pssim_sched_gen_random(int count);
int pssim_sched_run(pssim_sched_algo_t algo, int time_quantum);
void pssim_sched_print_gantt(void);
void pssim_sched_print_metrics(const pssim_metrics_t *metrics);
void pssim_sched_compare_all(int time_quantum);
int pssim_sched_load_file(const char *filename);
int pssim_sched_save_file(const char *filename);

/* Built-in Scheduler Commands integration */
int pssim_is_scheduler_builtin(const char *cmd);
int pssim_execute_scheduler_builtin(pssim_cmd_t *cmd);

#endif /* PSSIM_SCHEDULER_H */
