#include "../include/pssim_proc.h"
#include "../include/pssim_redirect.h"
#include "../include/pssim_signals.h"
#include "../include/pssim_threads.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int pssim_launch_process(char **argv, int is_background, const char *infile, const char *outfile, const char *appfile, const char *errfile) {
    if (!argv || !argv[0]) return 0;

    pid_t pid = fork();

    if (pid < 0) {
        perror("[PSSIM PROCESS CONTROL ERROR] Fork failed");
        return -1;
    } else if (pid == 0) {
        /* Child Process */
        pssim_cmd_t dummy_cmd;
        dummy_cmd.input_file = (char *)infile;
        dummy_cmd.output_file = (char *)outfile;
        dummy_cmd.append_file = (char *)appfile;
        dummy_cmd.error_file = (char *)errfile;

        int s_in, s_out, s_err;
        if (pssim_apply_redirection(&dummy_cmd, &s_in, &s_out, &s_err) < 0) {
            exit(EXIT_FAILURE);
        }

        if (execvp(argv[0], argv) < 0) {
            fprintf(stderr, "%sPSSIM: Command not found: %s%s\n", PSSIM_COLOR_RED, argv[0], PSSIM_COLOR_RESET);
            exit(EXIT_FAILURE);
        }
    } else {
        /* Parent Process */
        pssim_telemetry_inc_cmd();

        if (is_background) {
            printf("%s[PSSIM JOB CONTROL] Launched background process PID %d: %s%s\n", PSSIM_COLOR_YELLOW, pid, argv[0], PSSIM_COLOR_RESET);
            pssim_add_job(pid, argv[0]);
            g_shell_state.last_exit_status = 0;
        } else {
            int status = 0;
            waitpid(pid, &status, WUNTRACED);
            if (WIFEXITED(status)) {
                g_shell_state.last_exit_status = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                g_shell_state.last_exit_status = 128 + WTERMSIG(status);
            }
        }
    }
    return 0;
}

int pssim_execute_single_cmd(pssim_cmd_t *cmd) {
    if (!cmd || cmd->argc == 0) return 0;
    return pssim_launch_process(cmd->argv, cmd->is_background, cmd->input_file, cmd->output_file, cmd->append_file, cmd->error_file);
}
