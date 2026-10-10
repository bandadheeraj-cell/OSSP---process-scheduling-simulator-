#ifndef PSSIM_BUILTINS_H
#define PSSIM_BUILTINS_H

#include "pssim_core.h"
#include "pssim_parser.h"

int pssim_is_builtin(const char *cmd);
int pssim_execute_builtin(pssim_cmd_t *cmd);

/* Built-in function handlers */
int pssim_builtin_cd(char **argv);
int pssim_builtin_pwd(char **argv);
int pssim_builtin_help(char **argv);
int pssim_builtin_clear(char **argv);
int pssim_builtin_exit(char **argv);
int pssim_builtin_history(char **argv);
int pssim_builtin_echo(char **argv);
int pssim_builtin_env(char **argv);
int pssim_builtin_setenv(char **argv);
int pssim_builtin_unsetenv(char **argv);

#endif /* PSSIM_BUILTINS_H */
