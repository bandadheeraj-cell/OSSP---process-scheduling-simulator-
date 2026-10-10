#include "../include/pssim_pipes.h"
#include "../include/pssim_proc.h"
#include "../include/pssim_redirect.h"
#include "../include/pssim_signals.h"
#include "../include/pssim_threads.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int pssim_execute_pipeline(pssim_pipeline_t *pipeline) {
    if (!pipeline || pipeline->num_commands == 0) return 0;

    int num_cmds = pipeline->num_commands;
    if (num_cmds == 1) {
        return pssim_execute_single_cmd(&pipeline->commands[0]);
    }

    int pipefds[2 * (num_cmds - 1)];
    for (int i = 0; i < num_cmds - 1; i++) {
        if (pipe(pipefds + i * 2) < 0) {
            perror("[PSSIM IPC PIPE ERROR] Pipe allocation failed");
            return -1;
        }
    }

    pid_t pids[num_cmds];

    for (int i = 0; i < num_cmds; i++) {
        pssim_cmd_t *cmd = &pipeline->commands[i];
        pids[i] = fork();

        if (pids[i] < 0) {
            perror("[PSSIM IPC PIPE ERROR] Fork failed in pipeline");
            return -1;
        }

        if (pids[i] == 0) {
            /* Child Process */
            if (i > 0) {
                /* Redirect stdin from previous pipe read end */
                dup2(pipefds[(i - 1) * 2], STDIN_FILENO);
            }
            if (i < num_cmds - 1) {
                /* Redirect stdout to current pipe write end */
                dup2(pipefds[i * 2 + 1], STDOUT_FILENO);
            }

            /* Close all pipe descriptors in child */
            for (int j = 0; j < 2 * (num_cmds - 1); j++) {
                close(pipefds[j]);
            }

            /* Apply file redirections if specified for individual command */
            int s_in, s_out, s_err;
            if (pssim_apply_redirection(cmd, &s_in, &s_out, &s_err) < 0) {
                exit(EXIT_FAILURE);
            }

            if (execvp(cmd->argv[0], cmd->argv) < 0) {
                fprintf(stderr, "%sPSSIM Pipeline: Command execution failed: %s%s\n", PSSIM_COLOR_RED, cmd->argv[0], PSSIM_COLOR_RESET);
                exit(EXIT_FAILURE);
            }
        }
    }

    /* Parent Process: Close all pipe descriptors */
    for (int i = 0; i < 2 * (num_cmds - 1); i++) {
        close(pipefds[i]);
    }

    pssim_telemetry_inc_cmd();

    if (pipeline->is_background) {
        printf("%s[PSSIM JOB CONTROL] Pipeline launched in background.%s\n", PSSIM_COLOR_YELLOW, PSSIM_COLOR_RESET);
        for (int i = 0; i < num_cmds; i++) {
            pssim_add_job(pids[i], pipeline->commands[i].argv[0]);
        }
    } else {
        /* Wait for all child processes in the pipeline */
        int status = 0;
        for (int i = 0; i < num_cmds; i++) {
            waitpid(pids[i], &status, WUNTRACED);
            if (i == num_cmds - 1 && WIFEXITED(status)) {
                g_shell_state.last_exit_status = WEXITSTATUS(status);
            }
        }
    }

    return 0;
}
