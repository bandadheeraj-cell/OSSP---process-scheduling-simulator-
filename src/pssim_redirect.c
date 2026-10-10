#include "../include/pssim_redirect.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int pssim_apply_redirection(pssim_cmd_t *cmd, int *saved_stdin, int *saved_stdout, int *saved_stderr) {
    if (!cmd) return -1;

    *saved_stdin = -1;
    *saved_stdout = -1;
    *saved_stderr = -1;

    /* Input Redirection (<) */
    if (cmd->input_file) {
        int fd_in = open(cmd->input_file, O_RDONLY);
        if (fd_in < 0) {
            perror("[PSSIM I/O REDIRECT ERROR] Input file open failed");
            return -1;
        }
        *saved_stdin = dup(STDIN_FILENO);
        dup2(fd_in, STDIN_FILENO);
        close(fd_in);
    }

    /* Output Redirection (>) */
    if (cmd->output_file) {
        int fd_out = open(cmd->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_out < 0) {
            perror("[PSSIM I/O REDIRECT ERROR] Output file open failed");
            return -1;
        }
        *saved_stdout = dup(STDOUT_FILENO);
        dup2(fd_out, STDOUT_FILENO);
        close(fd_out);
    }

    /* Append Output Redirection (>>) */
    if (cmd->append_file) {
        int fd_app = open(cmd->append_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd_app < 0) {
            perror("[PSSIM I/O REDIRECT ERROR] Append file open failed");
            return -1;
        }
        *saved_stdout = dup(STDOUT_FILENO);
        dup2(fd_app, STDOUT_FILENO);
        close(fd_app);
    }

    /* Standard Error Redirection (2>) */
    if (cmd->error_file) {
        int fd_err = open(cmd->error_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_err < 0) {
            perror("[PSSIM I/O REDIRECT ERROR] Error file open failed");
            return -1;
        }
        *saved_stderr = dup(STDERR_FILENO);
        dup2(fd_err, STDERR_FILENO);
        close(fd_err);
    }

    return 0;
}

void pssim_restore_redirection(int saved_stdin, int saved_stdout, int saved_stderr) {
    if (saved_stdin != -1) {
        dup2(saved_stdin, STDIN_FILENO);
        close(saved_stdin);
    }
    if (saved_stdout != -1) {
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);
    }
    if (saved_stderr != -1) {
        dup2(saved_stderr, STDERR_FILENO);
        close(saved_stderr);
    }
}
