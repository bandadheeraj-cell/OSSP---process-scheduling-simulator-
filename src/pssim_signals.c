#include "../include/pssim_signals.h"
#include "../include/pssim_threads.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>

static pssim_job_t *g_job_list_head = NULL;
static int g_next_job_id = 1;

static void pssim_sigint_handler(int sig) {
    (void)sig;
    printf("\n%s[PSSIM SIGNAL] Interrupted (SIGINT). Shell remaining active.%s\n", PSSIM_COLOR_YELLOW, PSSIM_COLOR_RESET);
    pssim_print_prompt();
    fflush(stdout);
}

static void pssim_sigtstp_handler(int sig) {
    (void)sig;
    printf("\n%s[PSSIM SIGNAL] Stopped (SIGTSTP) ignored for shell REPL.%s\n", PSSIM_COLOR_YELLOW, PSSIM_COLOR_RESET);
    pssim_print_prompt();
    fflush(stdout);
}

static void pssim_sigchld_handler(int sig) {
    (void)sig;
    int saved_errno = errno;
    pid_t pid;
    int status;

    /* Reclaim all terminated background child processes without blocking */
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        pssim_remove_job(pid);
    }
    errno = saved_errno;
}

void pssim_setup_signals(void) {
    struct sigaction sa_int, sa_tstp, sa_chld;

    sa_int.sa_handler = pssim_sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa_int, NULL);

    sa_tstp.sa_handler = pssim_sigtstp_handler;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa_tstp, NULL);

    sa_chld.sa_handler = pssim_sigchld_handler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa_chld, NULL);
}

void pssim_add_job(pid_t pid, const char *command) {
    pssim_job_t *new_job = (pssim_job_t *)malloc(sizeof(pssim_job_t));
    if (!new_job) return;
    new_job->job_id = g_next_job_id++;
    new_job->pid = pid;
    strncpy(new_job->command, command ? command : "unknown", sizeof(new_job->command) - 1);
    new_job->command[sizeof(new_job->command) - 1] = '\0';
    new_job->is_running = 1;
    new_job->next = g_job_list_head;
    g_job_list_head = new_job;

    pthread_mutex_lock(&g_telemetry.lock);
    g_telemetry.active_background_jobs++;
    pthread_mutex_unlock(&g_telemetry.lock);
}

void pssim_remove_job(pid_t pid) {
    pssim_job_t **curr = &g_job_list_head;
    while (*curr) {
        if ((*curr)->pid == pid) {
            pssim_job_t *temp = *curr;
            *curr = (*curr)->next;
            free(temp);

            pthread_mutex_lock(&g_telemetry.lock);
            if (g_telemetry.active_background_jobs > 0) {
                g_telemetry.active_background_jobs--;
            }
            pthread_mutex_unlock(&g_telemetry.lock);
            return;
        }
        curr = &(*curr)->next;
    }
}

void pssim_print_jobs(void) {
    printf("\n%s=== Active Background Process Jobs ===%s\n", PSSIM_COLOR_CYAN, PSSIM_COLOR_RESET);
    pssim_job_t *curr = g_job_list_head;
    if (!curr) {
        printf("No active background jobs running.\n");
        return;
    }
    while (curr) {
        printf("[%d] PID: %d  State: RUNNING  Command: %s\n", curr->job_id, curr->pid, curr->command);
        curr = curr->next;
    }
}

int pssim_has_running_jobs(void) {
    return g_job_list_head != NULL;
}
