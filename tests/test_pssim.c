#include "../include/pssim_core.h"
#include "../include/pssim_scheduler.h"
#include "../include/pssim_parser.h"
#include "../include/pssim_threads.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

void test_scheduler_algorithms(void) {
    printf("[TEST] Initializing Scheduler Engine...\n");
    pssim_sched_init();
    pssim_sched_add_process("P1", 0, 8, 3);
    pssim_sched_add_process("P2", 1, 4, 1);
    pssim_sched_add_process("P3", 2, 9, 4);

    assert(g_scheduler.process_count == 3);

    printf("[TEST] Running FCFS Algorithm...\n");
    assert(pssim_sched_run(PSSIM_SCHED_FCFS, 2) == 0);
    assert(g_scheduler.last_metrics.avg_waiting_time > 0);

    printf("[TEST] Running SJF Algorithm...\n");
    assert(pssim_sched_run(PSSIM_SCHED_SJF, 2) == 0);

    printf("[TEST] Running SRTF Algorithm...\n");
    assert(pssim_sched_run(PSSIM_SCHED_SRTF, 2) == 0);

    printf("[TEST] Running Priority Non-Preemptive Algorithm...\n");
    assert(pssim_sched_run(PSSIM_SCHED_PRIORITY, 2) == 0);

    printf("[TEST] Running Priority Preemptive Algorithm...\n");
    assert(pssim_sched_run(PSSIM_SCHED_PRIORITY_P, 2) == 0);

    printf("[TEST] Running Round Robin Algorithm...\n");
    assert(pssim_sched_run(PSSIM_SCHED_RR, 2) == 0);

    printf("[TEST] Running MLFQ Algorithm...\n");
    assert(pssim_sched_run(PSSIM_SCHED_MLFQ, 2) == 0);

    printf("[TEST PASS] All 7 Scheduling Algorithms Executed Successfully!\n");
}

int main(void) {
    printf("=========================================\n");
    printf(" PSSIM AUTOMATED UNIT & SCHEDULER TESTS  \n");
    printf("=========================================\n");
    pssim_init_threads();
    test_scheduler_algorithms();
    pssim_cleanup_threads();
    printf("ALL TESTS PASSED CLEANLY!\n");
    return 0;
}
