#ifndef PSSIM_CORE_H
#define PSSIM_CORE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>

#define PSSIM_MAX_INPUT 1024
#define PSSIM_MAX_TOKENS 128
#define PSSIM_MAX_JOBS 64
#define PSSIM_COLOR_RESET   "\033[0m"
#define PSSIM_COLOR_RED     "\033[1;31m"
#define PSSIM_COLOR_GREEN   "\033[1;32m"
#define PSSIM_COLOR_YELLOW  "\033[1;33m"
#define PSSIM_COLOR_BLUE    "\033[1;34m"
#define PSSIM_COLOR_MAGENTA "\033[1;35m"
#define PSSIM_COLOR_CYAN    "\033[1;36m"
#define PSSIM_COLOR_BOLD    "\033[1m"

/* Shell State */
typedef struct {
    int running;
    int last_exit_status;
    char current_dir[1024];
    int history_count;
    char *history[100];
} pssim_shell_state_t;

extern pssim_shell_state_t g_shell_state;

void pssim_init_shell(void);
void pssim_cleanup_shell(void);
void pssim_print_prompt(void);
void pssim_add_history(const char *cmd);
void pssim_print_history(void);

#endif /* PSSIM_CORE_H */
