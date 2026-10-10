#ifndef PSSIM_SIGNALS_H
#define PSSIM_SIGNALS_H

#include "pssim_core.h"

typedef struct pssim_job {
    int job_id;
    pid_t pid;
    char command[256];
    int is_running;
    struct pssim_job *next;
} pssim_job_t;

void pssim_setup_signals(void);
void pssim_add_job(pid_t pid, const char *command);
void pssim_remove_job(pid_t pid);
void pssim_print_jobs(void);
int pssim_has_running_jobs(void);

#endif /* PSSIM_SIGNALS_H */
