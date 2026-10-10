#ifndef PSSIM_PROC_H
#define PSSIM_PROC_H

#include "pssim_core.h"
#include "pssim_parser.h"

int pssim_execute_single_cmd(pssim_cmd_t *cmd);
int pssim_launch_process(char **argv, int is_background, const char *infile, const char *outfile, const char *appfile, const char *errfile);

#endif /* PSSIM_PROC_H */
