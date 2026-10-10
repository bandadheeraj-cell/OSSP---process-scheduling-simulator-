#include "../include/pssim_core.h"
#include "../include/pssim_parser.h"
#include "../include/pssim_proc.h"
#include "../include/pssim_pipes.h"
#include "../include/pssim_builtins.h"
#include "../include/pssim_signals.h"
#include "../include/pssim_scheduler.h"
#include "../include/pssim_threads.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void pssim_print_welcome_banner(void) {
    printf("\n%s=========================================================================%s\n", PSSIM_COLOR_CYAN, PSSIM_COLOR_RESET);
    printf("%s   PROCESS SCHEDULING SIMULATOR WITH SHELL INTERFACE (PSSIM v1.0)       %s\n", PSSIM_COLOR_BOLD, PSSIM_COLOR_RESET);
    printf("%s   Operating Systems & Systems Programming Integrated Platform          %s\n", PSSIM_COLOR_GREEN, PSSIM_COLOR_RESET);
    printf("%s=========================================================================%s\n", PSSIM_COLOR_CYAN, PSSIM_COLOR_RESET);
    printf("Type '%shelp%s' for shell built-ins & scheduler simulator commands.\n", PSSIM_COLOR_YELLOW, PSSIM_COLOR_RESET);
    printf("Type '%ssched-gen 5%s' followed by '%ssched-compare 2%s' to run benchmarks.\n", PSSIM_COLOR_YELLOW, PSSIM_COLOR_RESET, PSSIM_COLOR_YELLOW, PSSIM_COLOR_RESET);
    printf("Type '%sexit%s' to terminate the session.\n\n", PSSIM_COLOR_YELLOW, PSSIM_COLOR_RESET);
}

int main(int argc, char **argv) {
    pssim_init_shell();
    pssim_setup_signals();
    pssim_init_threads();
    pssim_sched_init();

    /* Check for batch scheduling execution mode */
    if (argc >= 2 && (strcmp(argv[1], "--batch") == 0 || strcmp(argv[1], "-b") == 0)) {
        printf("%s[PSSIM BATCH MODE] Running Process Scheduling Benchmark...%s\n", PSSIM_COLOR_GREEN, PSSIM_COLOR_RESET);
        if (argc >= 3) {
            pssim_sched_load_file(argv[2]);
        } else {
            pssim_sched_gen_random(5);
        }
        int quantum = (argc >= 4) ? atoi(argv[3]) : 2;
        pssim_sched_compare_all(quantum);
        pssim_cleanup_threads();
        pssim_cleanup_shell();
        return 0;
    }

    pssim_print_welcome_banner();

    /* Interactive REPL Loop */
    while (g_shell_state.running) {
        pssim_print_prompt();

        char *line = pssim_read_line();
        if (!line) {
            printf("\nExiting shell REPL...\n");
            break;
        }

        char *trimmed = pssim_trim_whitespace(line);
        if (strlen(trimmed) == 0) {
            free(line);
            continue;
        }

        pssim_add_history(trimmed);

        pssim_pipeline_t *pipeline = pssim_parse_pipeline(trimmed);
        if (pipeline && pipeline->num_commands > 0) {
            if (pipeline->num_commands == 1 && pssim_is_builtin(pipeline->commands[0].argv[0])) {
                pssim_execute_builtin(&pipeline->commands[0]);
            } else {
                pssim_execute_pipeline(pipeline);
            }
            pssim_free_pipeline(pipeline);
        }

        free(line);
    }

    pssim_cleanup_threads();
    pssim_cleanup_shell();
    printf("%s[PSSIM SESSION TERMINATED] Goodbye!%s\n", PSSIM_COLOR_GREEN, PSSIM_COLOR_RESET);
    return 0;
}
