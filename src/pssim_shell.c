#include "../include/pssim_core.h"
#include "../include/pssim_parser.h"
#include "../include/pssim_proc.h"
#include "../include/pssim_pipes.h"
#include "../include/pssim_builtins.h"
#include "../include/pssim_signals.h"
#include "../include/pssim_scheduler.h"
#include "../include/pssim_threads.h"
#include "../include/pssim_memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

pssim_shell_state_t g_shell_state;

void pssim_init_shell(void) {
    g_shell_state.running = 1;
    g_shell_state.last_exit_status = 0;
    g_shell_state.history_count = 0;
    if (getcwd(g_shell_state.current_dir, sizeof(g_shell_state.current_dir)) == NULL) {
        snprintf(g_shell_state.current_dir, sizeof(g_shell_state.current_dir), ".");
    }
}

void pssim_cleanup_shell(void) {
    for (int i = 0; i < g_shell_state.history_count; i++) {
        if (g_shell_state.history[i]) {
            free(g_shell_state.history[i]);
        }
    }
    g_shell_state.history_count = 0;
}

void pssim_print_prompt(void) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        snprintf(cwd, sizeof(cwd), "%s", g_shell_state.current_dir);
    }
    printf("%sps_shell%s:%s%s%s$ ",
        PSSIM_COLOR_GREEN, PSSIM_COLOR_RESET,
        PSSIM_COLOR_BLUE, cwd, PSSIM_COLOR_RESET);
    fflush(stdout);
}

void pssim_add_history(const char *cmd) {
    if (!cmd || strlen(cmd) == 0) return;
    if (g_shell_state.history_count < 100) {
        g_shell_state.history[g_shell_state.history_count++] = pssim_strdup(cmd);
    } else {
        free(g_shell_state.history[0]);
        for (int i = 1; i < 100; i++) {
            g_shell_state.history[i - 1] = g_shell_state.history[i];
        }
        g_shell_state.history[99] = pssim_strdup(cmd);
    }
}

void pssim_print_history(void) {
    printf("\n%s=== Executed Command History ===%s\n", PSSIM_COLOR_CYAN, PSSIM_COLOR_RESET);
    for (int i = 0; i < g_shell_state.history_count; i++) {
        printf(" %3d  %s\n", i + 1, g_shell_state.history[i]);
    }
    printf("\n");
}
