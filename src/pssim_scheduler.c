#include "../include/pssim_scheduler.h"
#include "../include/pssim_memory.h"
#include "../include/pssim_threads.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

pssim_scheduler_t g_scheduler;

void pssim_sched_init(void) {
    g_scheduler.process_count = 0;
    g_scheduler.run_count = 0;
    g_scheduler.gantt_count = 0;
    g_scheduler.is_simulated = 0;
    memset(&g_scheduler.last_metrics, 0, sizeof(pssim_metrics_t));
}

void pssim_sched_reset(void) {
    g_scheduler.process_count = 0;
    g_scheduler.run_count = 0;
    g_scheduler.gantt_count = 0;
    g_scheduler.is_simulated = 0;
    memset(&g_scheduler.last_metrics, 0, sizeof(pssim_metrics_t));
    printf("%s[PSSIM SCHEDULER] Process queue and simulation results reset.%s\n", PSSIM_COLOR_GREEN, PSSIM_COLOR_RESET);
}

int pssim_sched_add_process(const char *name, int arrival, int burst, int priority) {
    if (g_scheduler.process_count >= PSSIM_MAX_SIM_PROCESSES) {
        fprintf(stderr, "%s[PSSIM SCHEDULER ERROR] Maximum process capacity reached.%s\n", PSSIM_COLOR_RED, PSSIM_COLOR_RESET);
        return -1;
    }
    int idx = g_scheduler.process_count++;
    pssim_pcb_t *proc = &g_scheduler.process_queue[idx];
    proc->pid = idx + 1;
    snprintf(proc->name, sizeof(proc->name), "%s", name ? name : "proc");
    proc->arrival_time = arrival >= 0 ? arrival : 0;
    proc->burst_time = burst > 0 ? burst : 1;
    proc->remaining_time = proc->burst_time;
    proc->priority = priority >= 0 ? priority : 1;
    proc->start_time = -1;
    proc->completion_time = 0;
    proc->turnaround_time = 0;
    proc->waiting_time = 0;
    proc->response_time = 0;
    proc->state = PSSIM_PROC_NEW;
    proc->queue_level = 0;
    proc->time_in_current_queue = 0;
    proc->last_executed_tick = arrival;
    return proc->pid;
}

void pssim_sched_list(void) {
    printf("\n%s=== Process Scheduling Simulator Workload Queue ===%s\n", PSSIM_COLOR_CYAN, PSSIM_COLOR_RESET);
    if (g_scheduler.process_count == 0) {
        printf("Queue is currently empty. Use 'sched-add' or 'sched-gen' to populate workload.\n");
        return;
    }
    printf("%-5s | %-12s | %-12s | %-10s | %-8s\n", "PID", "Name", "Arrival Time", "Burst Time", "Priority");
    printf("------------------------------------------------------------------\n");
    for (int i = 0; i < g_scheduler.process_count; i++) {
        pssim_pcb_t *p = &g_scheduler.process_queue[i];
        printf("%-5d | %-12s | %-12d | %-10d | %-8d\n", p->pid, p->name, p->arrival_time, p->burst_time, p->priority);
    }
    printf("Total Processes: %d\n\n", g_scheduler.process_count);
}

void pssim_sched_gen_random(int count) {
    pssim_sched_reset();
    if (count <= 0) count = 5;
    if (count > PSSIM_MAX_SIM_PROCESSES) count = PSSIM_MAX_SIM_PROCESSES;

    srand((unsigned int)time(NULL));
    for (int i = 0; i < count; i++) {
        char pname[16];
        snprintf(pname, sizeof(pname), "P%d", i + 1);
        int arr = rand() % 8;
        int burst = (rand() % 10) + 1;
        int prio = (rand() % 5) + 1;
        pssim_sched_add_process(pname, arr, burst, prio);
    }
    printf("%s[PSSIM SCHEDULER] Generated %d random process workloads.%s\n", PSSIM_COLOR_GREEN, count, PSSIM_COLOR_RESET);
}

static void add_gantt_entry(int pid, const char *name, int start, int end) {
    if (g_scheduler.gantt_count > 0) {
        pssim_gantt_entry_t *last = &g_scheduler.gantt[g_scheduler.gantt_count - 1];
        if (last->pid == pid && last->end_time == start) {
            last->end_time = end;
            return;
        }
    }
    if (g_scheduler.gantt_count < PSSIM_MAX_GANTT_ENTRIES) {
        pssim_gantt_entry_t *entry = &g_scheduler.gantt[g_scheduler.gantt_count++];
        entry->pid = pid;
        snprintf(entry->name, sizeof(entry->name), "%s", name);
        entry->start_time = start;
        entry->end_time = end;
    }
}

/* 1. FCFS (First-Come First-Served) */
static void run_fcfs(pssim_pcb_t *procs, int count) {
    /* Sort by arrival time */
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (procs[j].arrival_time > procs[j + 1].arrival_time) {
                pssim_pcb_t temp = procs[j];
                procs[j] = procs[j + 1];
                procs[j + 1] = temp;
            }
        }
    }

    int current_time = 0;
    for (int i = 0; i < count; i++) {
        if (current_time < procs[i].arrival_time) {
            add_gantt_entry(-1, "IDLE", current_time, procs[i].arrival_time);
            current_time = procs[i].arrival_time;
        }
        procs[i].start_time = current_time;
        procs[i].completion_time = current_time + procs[i].burst_time;
        procs[i].turnaround_time = procs[i].completion_time - procs[i].arrival_time;
        procs[i].waiting_time = procs[i].turnaround_time - procs[i].burst_time;
        procs[i].response_time = procs[i].start_time - procs[i].arrival_time;
        add_gantt_entry(procs[i].pid, procs[i].name, current_time, procs[i].completion_time);
        current_time = procs[i].completion_time;
    }
}

/* 2. SJF (Non-preemptive Shortest Job First) */
static void run_sjf(pssim_pcb_t *procs, int count) {
    int completed = 0;
    int current_time = 0;
    int is_completed[count];
    memset(is_completed, 0, sizeof(is_completed));

    while (completed < count) {
        int idx = -1;
        int min_burst = 1e9;
        for (int i = 0; i < count; i++) {
            if (procs[i].arrival_time <= current_time && !is_completed[i]) {
                if (procs[i].burst_time < min_burst) {
                    min_burst = procs[i].burst_time;
                    idx = i;
                } else if (procs[i].burst_time == min_burst) {
                    if (procs[i].arrival_time < procs[idx].arrival_time) {
                        idx = i;
                    }
                }
            }
        }

        if (idx != -1) {
            procs[idx].start_time = current_time;
            procs[idx].completion_time = current_time + procs[idx].burst_time;
            procs[idx].turnaround_time = procs[idx].completion_time - procs[idx].arrival_time;
            procs[idx].waiting_time = procs[idx].turnaround_time - procs[idx].burst_time;
            procs[idx].response_time = procs[idx].start_time - procs[idx].arrival_time;
            add_gantt_entry(procs[idx].pid, procs[idx].name, current_time, procs[idx].completion_time);
            current_time = procs[idx].completion_time;
            is_completed[idx] = 1;
            completed++;
        } else {
            add_gantt_entry(-1, "IDLE", current_time, current_time + 1);
            current_time++;
        }
    }
}

/* 3. SRTF (Preemptive Shortest Remaining Time First) */
static void run_srtf(pssim_pcb_t *procs, int count) {
    int completed = 0;
    int current_time = 0;

    while (completed < count) {
        int idx = -1;
        int min_remaining = 1e9;
        for (int i = 0; i < count; i++) {
            if (procs[i].arrival_time <= current_time && procs[i].remaining_time > 0) {
                if (procs[i].remaining_time < min_remaining) {
                    min_remaining = procs[i].remaining_time;
                    idx = i;
                }
            }
        }

        if (idx != -1) {
            if (procs[idx].start_time == -1) {
                procs[idx].start_time = current_time;
            }
            procs[idx].remaining_time--;
            add_gantt_entry(procs[idx].pid, procs[idx].name, current_time, current_time + 1);
            current_time++;

            if (procs[idx].remaining_time == 0) {
                procs[idx].completion_time = current_time;
                procs[idx].turnaround_time = procs[idx].completion_time - procs[idx].arrival_time;
                procs[idx].waiting_time = procs[idx].turnaround_time - procs[idx].burst_time;
                procs[idx].response_time = procs[idx].start_time - procs[idx].arrival_time;
                completed++;
            }
        } else {
            add_gantt_entry(-1, "IDLE", current_time, current_time + 1);
            current_time++;
        }
    }
}

/* 4. Priority Non-Preemptive */
static void run_priority_np(pssim_pcb_t *procs, int count) {
    int completed = 0;
    int current_time = 0;
    int is_completed[count];
    memset(is_completed, 0, sizeof(is_completed));

    while (completed < count) {
        int idx = -1;
        int highest_prio = 1e9;
        for (int i = 0; i < count; i++) {
            if (procs[i].arrival_time <= current_time && !is_completed[i]) {
                if (procs[i].priority < highest_prio) {
                    highest_prio = procs[i].priority;
                    idx = i;
                }
            }
        }

        if (idx != -1) {
            procs[idx].start_time = current_time;
            procs[idx].completion_time = current_time + procs[idx].burst_time;
            procs[idx].turnaround_time = procs[idx].completion_time - procs[idx].arrival_time;
            procs[idx].waiting_time = procs[idx].turnaround_time - procs[idx].burst_time;
            procs[idx].response_time = procs[idx].start_time - procs[idx].arrival_time;
            add_gantt_entry(procs[idx].pid, procs[idx].name, current_time, procs[idx].completion_time);
            current_time = procs[idx].completion_time;
            is_completed[idx] = 1;
            completed++;
        } else {
            add_gantt_entry(-1, "IDLE", current_time, current_time + 1);
            current_time++;
        }
    }
}

/* 5. Priority Preemptive */
static void run_priority_p(pssim_pcb_t *procs, int count) {
    int completed = 0;
    int current_time = 0;

    while (completed < count) {
        int idx = -1;
        int highest_prio = 1e9;
        for (int i = 0; i < count; i++) {
            if (procs[i].arrival_time <= current_time && procs[i].remaining_time > 0) {
                if (procs[i].priority < highest_prio) {
                    highest_prio = procs[i].priority;
                    idx = i;
                }
            }
        }

        if (idx != -1) {
            if (procs[idx].start_time == -1) {
                procs[idx].start_time = current_time;
            }
            procs[idx].remaining_time--;
            add_gantt_entry(procs[idx].pid, procs[idx].name, current_time, current_time + 1);
            current_time++;

            if (procs[idx].remaining_time == 0) {
                procs[idx].completion_time = current_time;
                procs[idx].turnaround_time = procs[idx].completion_time - procs[idx].arrival_time;
                procs[idx].waiting_time = procs[idx].turnaround_time - procs[idx].burst_time;
                procs[idx].response_time = procs[idx].start_time - procs[idx].arrival_time;
                completed++;
            }
        } else {
            add_gantt_entry(-1, "IDLE", current_time, current_time + 1);
            current_time++;
        }
    }
}

/* 6. Round Robin (RR) */
static void run_rr(pssim_pcb_t *procs, int count, int quantum) {
    if (quantum <= 0) quantum = 2;

    int queue[PSSIM_MAX_SIM_PROCESSES * 10];
    int front = 0, rear = 0;

    int current_time = 0;
    int completed = 0;
    int visited[count];
    memset(visited, 0, sizeof(visited));

    /* Find earliest arrival */
    int min_arr = 1e9;
    for (int i = 0; i < count; i++) {
        if (procs[i].arrival_time < min_arr) min_arr = procs[i].arrival_time;
    }
    current_time = min_arr;

    for (int i = 0; i < count; i++) {
        if (procs[i].arrival_time == current_time) {
            queue[rear++] = i;
            visited[i] = 1;
        }
    }

    while (completed < count) {
        if (front == rear) {
            /* Queue empty, check next process arrival */
            int next_arr = 1e9;
            for (int i = 0; i < count; i++) {
                if (!visited[i] && procs[i].arrival_time < next_arr) {
                    next_arr = procs[i].arrival_time;
                }
            }
            if (next_arr != 1e9 && next_arr > current_time) {
                add_gantt_entry(-1, "IDLE", current_time, next_arr);
                current_time = next_arr;
                for (int i = 0; i < count; i++) {
                    if (!visited[i] && procs[i].arrival_time <= current_time) {
                        queue[rear++] = i;
                        visited[i] = 1;
                    }
                }
            } else {
                current_time++;
                continue;
            }
        }

        int idx = queue[front++];
        if (procs[idx].start_time == -1) {
            procs[idx].start_time = current_time;
        }

        int exec_time = (procs[idx].remaining_time < quantum) ? procs[idx].remaining_time : quantum;
        procs[idx].remaining_time -= exec_time;
        add_gantt_entry(procs[idx].pid, procs[idx].name, current_time, current_time + exec_time);
        current_time += exec_time;

        /* Enqueue new arrivals during exec_time */
        for (int i = 0; i < count; i++) {
            if (!visited[i] && procs[i].arrival_time <= current_time) {
                queue[rear++] = i;
                visited[i] = 1;
            }
        }

        if (procs[idx].remaining_time > 0) {
            queue[rear++] = idx;
        } else {
            procs[idx].completion_time = current_time;
            procs[idx].turnaround_time = procs[idx].completion_time - procs[idx].arrival_time;
            procs[idx].waiting_time = procs[idx].turnaround_time - procs[idx].burst_time;
            procs[idx].response_time = procs[idx].start_time - procs[idx].arrival_time;
            completed++;
        }
    }
}

/* 7. MLFQ (Multi-Level Feedback Queue with Aging) */
static void run_mlfq(pssim_pcb_t *procs, int count) {
    int completed = 0;
    int current_time = 0;
    int q0_quantum = 2;
    int q1_quantum = 4;

    while (completed < count) {
        /* Aging mechanism: promote processes stuck in Q1/Q2 for >= 10 ticks */
        for (int i = 0; i < count; i++) {
            if (procs[i].arrival_time <= current_time && procs[i].remaining_time > 0) {
                if (procs[i].queue_level > 0 && (current_time - procs[i].last_executed_tick) >= 10) {
                    procs[i].queue_level--;
                    procs[i].last_executed_tick = current_time;
                }
            }
        }

        int idx = -1;
        /* Select process from highest priority level available (Queue 0 > Queue 1 > Queue 2) */
        for (int q = 0; q <= 2; q++) {
            for (int i = 0; i < count; i++) {
                if (procs[i].arrival_time <= current_time && procs[i].remaining_time > 0 && procs[i].queue_level == q) {
                    idx = i;
                    break;
                }
            }
            if (idx != -1) break;
        }

        if (idx != -1) {
            if (procs[idx].start_time == -1) {
                procs[idx].start_time = current_time;
            }

            int quantum = (procs[idx].queue_level == 0) ? q0_quantum : (procs[idx].queue_level == 1) ? q1_quantum : procs[idx].remaining_time;
            int exec_time = (procs[idx].remaining_time < quantum) ? procs[idx].remaining_time : quantum;

            procs[idx].remaining_time -= exec_time;
            procs[idx].last_executed_tick = current_time + exec_time;
            add_gantt_entry(procs[idx].pid, procs[idx].name, current_time, current_time + exec_time);
            current_time += exec_time;

            if (procs[idx].remaining_time == 0) {
                procs[idx].completion_time = current_time;
                procs[idx].turnaround_time = procs[idx].completion_time - procs[idx].arrival_time;
                procs[idx].waiting_time = procs[idx].turnaround_time - procs[idx].burst_time;
                procs[idx].response_time = procs[idx].start_time - procs[idx].arrival_time;
                completed++;
            } else {
                /* Demote process to next lower priority queue if it used full quantum */
                if (procs[idx].queue_level < 2 && exec_time == quantum) {
                    procs[idx].queue_level++;
                }
            }
        } else {
            add_gantt_entry(-1, "IDLE", current_time, current_time + 1);
            current_time++;
        }
    }
}

static void calculate_metrics(pssim_pcb_t *procs, int count, pssim_metrics_t *metrics, const char *algo_name) {
    snprintf(metrics->algo_name, sizeof(metrics->algo_name), "%s", algo_name);
    metrics->process_count = count;
    double total_wait = 0, total_turnaround = 0, total_response = 0;
    int max_completion = 0, min_arrival = 1e9, total_burst = 0;

    for (int i = 0; i < count; i++) {
        total_wait += procs[i].waiting_time;
        total_turnaround += procs[i].turnaround_time;
        total_response += procs[i].response_time;
        total_burst += procs[i].burst_time;
        if (procs[i].completion_time > max_completion) max_completion = procs[i].completion_time;
        if (procs[i].arrival_time < min_arrival) min_arrival = procs[i].arrival_time;
    }

    metrics->avg_waiting_time = count > 0 ? (total_wait / count) : 0;
    metrics->avg_turnaround_time = count > 0 ? (total_turnaround / count) : 0;
    metrics->avg_response_time = count > 0 ? (total_response / count) : 0;
    metrics->total_execution_ticks = max_completion - min_arrival;
    metrics->busy_ticks = total_burst;
    metrics->cpu_utilization = metrics->total_execution_ticks > 0 ? ((double)metrics->busy_ticks / metrics->total_execution_ticks) * 100.0 : 100.0;
    metrics->throughput = metrics->total_execution_ticks > 0 ? ((double)count / metrics->total_execution_ticks) : 0;
    metrics->total_context_switches = g_scheduler.gantt_count;
}

int pssim_sched_run(pssim_sched_algo_t algo, int time_quantum) {
    if (g_scheduler.process_count == 0) {
        fprintf(stderr, "%s[PSSIM SCHEDULER ERROR] No processes in queue. Use 'sched-add' or 'sched-gen' first.%s\n", PSSIM_COLOR_RED, PSSIM_COLOR_RESET);
        return -1;
    }

    g_scheduler.gantt_count = 0;
    g_scheduler.run_count = g_scheduler.process_count;
    memcpy(g_scheduler.run_queue, g_scheduler.process_queue, sizeof(pssim_pcb_t) * g_scheduler.process_count);

    const char *algo_name = "";
    switch (algo) {
        case PSSIM_SCHED_FCFS:
            algo_name = "First-Come First-Served (FCFS)";
            run_fcfs(g_scheduler.run_queue, g_scheduler.run_count);
            break;
        case PSSIM_SCHED_SJF:
            algo_name = "Shortest Job First (SJF Non-Preemptive)";
            run_sjf(g_scheduler.run_queue, g_scheduler.run_count);
            break;
        case PSSIM_SCHED_SRTF:
            algo_name = "Shortest Remaining Time First (SRTF Preemptive)";
            run_srtf(g_scheduler.run_queue, g_scheduler.run_count);
            break;
        case PSSIM_SCHED_PRIORITY:
            algo_name = "Priority Scheduling (Non-Preemptive)";
            run_priority_np(g_scheduler.run_queue, g_scheduler.run_count);
            break;
        case PSSIM_SCHED_PRIORITY_P:
            algo_name = "Priority Scheduling (Preemptive)";
            run_priority_p(g_scheduler.run_queue, g_scheduler.run_count);
            break;
        case PSSIM_SCHED_RR:
            algo_name = "Round Robin (RR)";
            run_rr(g_scheduler.run_queue, g_scheduler.run_count, time_quantum);
            break;
        case PSSIM_SCHED_MLFQ:
            algo_name = "Multi-Level Feedback Queue (MLFQ)";
            run_mlfq(g_scheduler.run_queue, g_scheduler.run_count);
            break;
    }

    calculate_metrics(g_scheduler.run_queue, g_scheduler.run_count, &g_scheduler.last_metrics, algo_name);
    g_scheduler.is_simulated = 1;
    pssim_telemetry_inc_sched();

    printf("%s[PSSIM SCHEDULER] Executed simulation algorithm: %s%s\n", PSSIM_COLOR_GREEN, algo_name, PSSIM_COLOR_RESET);
    pssim_sched_print_metrics(&g_scheduler.last_metrics);
    return 0;
}

void pssim_sched_print_gantt(void) {
    if (!g_scheduler.is_simulated || g_scheduler.gantt_count == 0) {
        printf("%sNo simulation results available. Run 'sched-run' first.%s\n", PSSIM_COLOR_YELLOW, PSSIM_COLOR_RESET);
        return;
    }

    printf("\n%s=== ASCII Gantt Chart Execution Timeline (%s) ===%s\n", PSSIM_COLOR_CYAN, g_scheduler.last_metrics.algo_name, PSSIM_COLOR_RESET);

    /* Render Top Border */
    printf("+");
    for (int i = 0; i < g_scheduler.gantt_count; i++) {
        int width = (g_scheduler.gantt[i].end_time - g_scheduler.gantt[i].start_time) * 3 + 2;
        if (width < 6) width = 6;
        for (int w = 0; w < width; w++) printf("-");
        printf("+");
    }
    printf("\n|");

    /* Render Labels */
    for (int i = 0; i < g_scheduler.gantt_count; i++) {
        int width = (g_scheduler.gantt[i].end_time - g_scheduler.gantt[i].start_time) * 3 + 2;
        if (width < 6) width = 6;
        char label[16];
        if (g_scheduler.gantt[i].pid == -1) {
            snprintf(label, sizeof(label), "IDLE");
        } else {
            snprintf(label, sizeof(label), "%s", g_scheduler.gantt[i].name);
        }
        int pad = (width - (int)strlen(label)) / 2;
        for (int p = 0; p < pad; p++) printf(" ");
        printf("%s", label);
        for (int p = 0; p < width - pad - (int)strlen(label); p++) printf(" ");
        printf("|");
    }
    printf("\n+");

    /* Render Bottom Border */
    for (int i = 0; i < g_scheduler.gantt_count; i++) {
        int width = (g_scheduler.gantt[i].end_time - g_scheduler.gantt[i].start_time) * 3 + 2;
        if (width < 6) width = 6;
        for (int w = 0; w < width; w++) printf("-");
        printf("+");
    }
    printf("\n");

    /* Render Timeline Numbers */
    printf("%d", g_scheduler.gantt[0].start_time);
    for (int i = 0; i < g_scheduler.gantt_count; i++) {
        int width = (g_scheduler.gantt[i].end_time - g_scheduler.gantt[i].start_time) * 3 + 2;
        if (width < 6) width = 6;
        for (int w = 0; w < width - 1; w++) printf(" ");
        printf("%-2d", g_scheduler.gantt[i].end_time);
    }
    printf("\n\n");
}

void pssim_sched_print_metrics(const pssim_metrics_t *metrics) {
    if (!metrics) return;
    printf("\n%s=== Performance Metrics Summary (%s) ===%s\n", PSSIM_COLOR_MAGENTA, metrics->algo_name, PSSIM_COLOR_RESET);
    printf("Average Waiting Time (AWT):    %.2f ticks\n", metrics->avg_waiting_time);
    printf("Average Turnaround Time (ATT): %.2f ticks\n", metrics->avg_turnaround_time);
    printf("Average Response Time (ART):   %.2f ticks\n", metrics->avg_response_time);
    printf("CPU Utilization:               %.2f %%\n", metrics->cpu_utilization);
    printf("Throughput:                    %.3f processes/tick\n", metrics->throughput);
    printf("Total Context Switches:        %d\n", metrics->total_context_switches);
    printf("---------------------------------------------------\n\n");
}

void pssim_sched_compare_all(int time_quantum) {
    if (g_scheduler.process_count == 0) {
        fprintf(stderr, "%s[PSSIM SCHEDULER ERROR] No process workload loaded for comparison.%s\n", PSSIM_COLOR_RED, PSSIM_COLOR_RESET);
        return;
    }

    pssim_metrics_t results[7];
    pssim_sched_algo_t algos[7] = {
        PSSIM_SCHED_FCFS, PSSIM_SCHED_SJF, PSSIM_SCHED_SRTF,
        PSSIM_SCHED_PRIORITY, PSSIM_SCHED_PRIORITY_P, PSSIM_SCHED_RR, PSSIM_SCHED_MLFQ
    };

    for (int i = 0; i < 7; i++) {
        pssim_sched_run(algos[i], time_quantum);
        results[i] = g_scheduler.last_metrics;
    }

    printf("\n%s===================================================================================================%s\n", PSSIM_COLOR_CYAN, PSSIM_COLOR_RESET);
    printf("%s               PROCESS SCHEDULING ALGORITHMS BENCHMARK COMPARATIVE MATRIX                        %s\n", PSSIM_COLOR_BOLD, PSSIM_COLOR_RESET);
    printf("%s===================================================================================================%s\n", PSSIM_COLOR_CYAN, PSSIM_COLOR_RESET);
    printf("%-35s | %-10s | %-10s | %-10s | %-10s\n", "Algorithm", "Avg Wait", "Avg Turn", "Avg Resp", "CPU Util %");
    printf("---------------------------------------------------------------------------------------------------\n");

    int best_idx = 0;
    double min_wait = 1e9;

    for (int i = 0; i < 7; i++) {
        printf("%-35s | %-10.2f | %-10.2f | %-10.2f | %-10.2f%%\n",
            results[i].algo_name, results[i].avg_waiting_time, results[i].avg_turnaround_time,
            results[i].avg_response_time, results[i].cpu_utilization);
        if (results[i].avg_waiting_time < min_wait) {
            min_wait = results[i].avg_waiting_time;
            best_idx = i;
        }
    }
    printf("---------------------------------------------------------------------------------------------------\n");
    printf("%sOptimal Algorithm for Current Workload: %s (Lowest Avg Wait: %.2f ticks)%s\n\n",
        PSSIM_COLOR_GREEN, results[best_idx].algo_name, results[best_idx].avg_waiting_time, PSSIM_COLOR_RESET);
}

int pssim_sched_load_file(const char *filename) {
    if (!filename) return -1;
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("[PSSIM FILE ERROR] Unable to open workload file");
        return -1;
    }
    pssim_sched_reset();
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        char name[32];
        int arr, burst, prio;
        if (sscanf(line, "%31s %d %d %d", name, &arr, &burst, &prio) == 4) {
            pssim_sched_add_process(name, arr, burst, prio);
        } else if (sscanf(line, "%31s %d %d", name, &arr, &burst) == 3) {
            pssim_sched_add_process(name, arr, burst, 1);
        }
    }
    fclose(fp);
    printf("%s[PSSIM SCHEDULER] Successfully loaded %d process workloads from '%s'.%s\n",
        PSSIM_COLOR_GREEN, g_scheduler.process_count, filename, PSSIM_COLOR_RESET);
    return 0;
}

int pssim_sched_save_file(const char *filename) {
    if (!filename) return -1;
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("[PSSIM FILE ERROR] Unable to open file for writing");
        return -1;
    }
    fprintf(fp, "# Process Workload File (Generated by PSSIM)\n");
    fprintf(fp, "# Name Arrival Burst Priority\n");
    for (int i = 0; i < g_scheduler.process_count; i++) {
        pssim_pcb_t *p = &g_scheduler.process_queue[i];
        fprintf(fp, "%s %d %d %d\n", p->name, p->arrival_time, p->burst_time, p->priority);
    }
    fclose(fp);
    printf("%s[PSSIM SCHEDULER] Successfully saved workload to '%s'.%s\n", PSSIM_COLOR_GREEN, filename, PSSIM_COLOR_RESET);
    return 0;
}

int pssim_is_scheduler_builtin(const char *cmd) {
    if (!cmd) return 0;
    return (strncmp(cmd, "sched-", 6) == 0);
}

int pssim_execute_scheduler_builtin(pssim_cmd_t *cmd) {
    if (!cmd || !cmd->argv || !cmd->argv[0]) return 0;
    char *name = cmd->argv[0];

    if (strcmp(name, "sched-reset") == 0) {
        pssim_sched_reset();
        return 0;
    }
    if (strcmp(name, "sched-add") == 0) {
        if (cmd->argc < 4) {
            fprintf(stderr, "%sUsage: sched-add <name> <arrival> <burst> [priority]%s\n", PSSIM_COLOR_RED, PSSIM_COLOR_RESET);
            return 1;
        }
        int arr = atoi(cmd->argv[2]);
        int burst = atoi(cmd->argv[3]);
        int prio = (cmd->argc >= 5) ? atoi(cmd->argv[4]) : 1;
        pssim_sched_add_process(cmd->argv[1], arr, burst, prio);
        return 0;
    }
    if (strcmp(name, "sched-list") == 0) {
        pssim_sched_list();
        return 0;
    }
    if (strcmp(name, "sched-gen") == 0) {
        int count = (cmd->argc >= 2) ? atoi(cmd->argv[1]) : 5;
        pssim_sched_gen_random(count);
        return 0;
    }
    if (strcmp(name, "sched-run") == 0) {
        if (cmd->argc < 2) {
            fprintf(stderr, "%sUsage: sched-run <fcfs|sjf|srtf|priority|priority_p|rr|mlfq> [quantum]%s\n", PSSIM_COLOR_RED, PSSIM_COLOR_RESET);
            return 1;
        }
        char *algo_str = cmd->argv[1];
        int quantum = (cmd->argc >= 3) ? atoi(cmd->argv[2]) : 2;
        pssim_sched_algo_t algo = PSSIM_SCHED_FCFS;
        if (strcasecmp(algo_str, "fcfs") == 0) algo = PSSIM_SCHED_FCFS;
        else if (strcasecmp(algo_str, "sjf") == 0) algo = PSSIM_SCHED_SJF;
        else if (strcasecmp(algo_str, "srtf") == 0) algo = PSSIM_SCHED_SRTF;
        else if (strcasecmp(algo_str, "priority") == 0) algo = PSSIM_SCHED_PRIORITY;
        else if (strcasecmp(algo_str, "priority_p") == 0) algo = PSSIM_SCHED_PRIORITY_P;
        else if (strcasecmp(algo_str, "rr") == 0) algo = PSSIM_SCHED_RR;
        else if (strcasecmp(algo_str, "mlfq") == 0) algo = PSSIM_SCHED_MLFQ;
        else {
            fprintf(stderr, "%sUnknown algorithm '%s'. Defaulting to FCFS.%s\n", PSSIM_COLOR_YELLOW, algo_str, PSSIM_COLOR_RESET);
        }
        pssim_sched_run(algo, quantum);
        return 0;
    }
    if (strcmp(name, "sched-gantt") == 0) {
        pssim_sched_print_gantt();
        return 0;
    }
    if (strcmp(name, "sched-metrics") == 0) {
        pssim_sched_print_metrics(&g_scheduler.last_metrics);
        return 0;
    }
    if (strcmp(name, "sched-compare") == 0) {
        int quantum = (cmd->argc >= 2) ? atoi(cmd->argv[1]) : 2;
        pssim_sched_compare_all(quantum);
        return 0;
    }
    if (strcmp(name, "sched-load") == 0) {
        if (cmd->argc < 2) {
            fprintf(stderr, "%sUsage: sched-load <filename>%s\n", PSSIM_COLOR_RED, PSSIM_COLOR_RESET);
            return 1;
        }
        pssim_sched_load_file(cmd->argv[1]);
        return 0;
    }
    if (strcmp(name, "sched-save") == 0) {
        if (cmd->argc < 2) {
            fprintf(stderr, "%sUsage: sched-save <filename>%s\n", PSSIM_COLOR_RED, PSSIM_COLOR_RESET);
            return 1;
        }
        pssim_sched_save_file(cmd->argv[1]);
        return 0;
    }
    return -1;
}
