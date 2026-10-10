#ifndef PSSIM_REDIRECT_H
#define PSSIM_REDIRECT_H

#include "pssim_core.h"
#include "pssim_parser.h"

int pssim_apply_redirection(pssim_cmd_t *cmd, int *saved_stdin, int *saved_stdout, int *saved_stderr);
void pssim_restore_redirection(int saved_stdin, int saved_stdout, int saved_stderr);

#endif /* PSSIM_REDIRECT_H */
