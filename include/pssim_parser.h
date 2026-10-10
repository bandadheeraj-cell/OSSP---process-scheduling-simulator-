#ifndef PSSIM_PARSER_H
#define PSSIM_PARSER_H

#include "pssim_core.h"

typedef struct {
    char **argv;
    int argc;
    char *input_file;   /* < input redirection */
    char *output_file;  /* > output redirection */
    char *append_file;  /* >> append redirection */
    char *error_file;   /* 2> error redirection */
    int is_background;  /* & background execution flag */
} pssim_cmd_t;

typedef struct {
    pssim_cmd_t *commands;
    int num_commands;
    int is_background;
} pssim_pipeline_t;

char *pssim_read_line(void);
pssim_pipeline_t *pssim_parse_pipeline(const char *input);
void pssim_free_pipeline(pssim_pipeline_t *pipeline);
char *pssim_trim_whitespace(char *str);

#endif /* PSSIM_PARSER_H */
