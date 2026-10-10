#include "../include/pssim_parser.h"
#include "../include/pssim_memory.h"
#include <ctype.h>

char *pssim_trim_whitespace(char *str) {
    if (!str) return NULL;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

char *pssim_read_line(void) {
    size_t bufsize = PSSIM_MAX_INPUT;
    char *buffer = (char *)pssim_malloc(bufsize * sizeof(char));
    size_t position = 0;

    while (1) {
        int c = getchar();
        if (c == EOF || c == '\n') {
            if (c == EOF && position == 0) {
                free(buffer);
                return NULL;
            }
            buffer[position] = '\0';
            return buffer;
        } else {
            buffer[position] = (char)c;
        }
        position++;

        if (position >= bufsize) {
            bufsize += PSSIM_MAX_INPUT;
            buffer = (char *)pssim_realloc(buffer, bufsize * sizeof(char));
        }
    }
}

static void pssim_parse_single_cmd(char *raw_cmd, pssim_cmd_t *cmd) {
    cmd->argv = (char **)pssim_malloc(PSSIM_MAX_TOKENS * sizeof(char *));
    cmd->argc = 0;
    cmd->input_file = NULL;
    cmd->output_file = NULL;
    cmd->append_file = NULL;
    cmd->error_file = NULL;
    cmd->is_background = 0;

    char *str = pssim_strdup(raw_cmd);
    char *p = str;

    /* Check for background execution flag '&' at end */
    size_t len = strlen(p);
    while (len > 0 && isspace((unsigned char)p[len - 1])) {
        p[--len] = '\0';
    }
    if (len > 0 && p[len - 1] == '&') {
        cmd->is_background = 1;
        p[len - 1] = '\0';
    }

    /* Tokenize command line preserving quotes or operator tokens */
    char *token = strtok(p, " \t\r\n");
    while (token != NULL) {
        if (strcmp(token, "<") == 0) {
            token = strtok(NULL, " \t\r\n");
            if (token) cmd->input_file = pssim_strdup(token);
        } else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " \t\r\n");
            if (token) cmd->output_file = pssim_strdup(token);
        } else if (strcmp(token, ">>") == 0) {
            token = strtok(NULL, " \t\r\n");
            if (token) cmd->append_file = pssim_strdup(token);
        } else if (strcmp(token, "2>") == 0) {
            token = strtok(NULL, " \t\r\n");
            if (token) cmd->error_file = pssim_strdup(token);
        } else {
            cmd->argv[cmd->argc++] = pssim_strdup(token);
            if (cmd->argc >= PSSIM_MAX_TOKENS - 1) break;
        }
        token = strtok(NULL, " \t\r\n");
    }
    cmd->argv[cmd->argc] = NULL;
    free(str);
}

pssim_pipeline_t *pssim_parse_pipeline(const char *input) {
    if (!input) return NULL;
    char *copy = pssim_strdup(input);
    char *trimmed = pssim_trim_whitespace(copy);
    if (strlen(trimmed) == 0) {
        free(copy);
        return NULL;
    }

    pssim_pipeline_t *pipeline = (pssim_pipeline_t *)pssim_malloc(sizeof(pssim_pipeline_t));
    pipeline->commands = (pssim_cmd_t *)pssim_malloc(PSSIM_MAX_TOKENS * sizeof(pssim_cmd_t));
    pipeline->num_commands = 0;
    pipeline->is_background = 0;

    char *saveptr = NULL;
    char *pipe_str = strtok_r(trimmed, "|", &saveptr);
    while (pipe_str != NULL) {
        pssim_parse_single_cmd(pipe_str, &pipeline->commands[pipeline->num_commands]);
        if (pipeline->commands[pipeline->num_commands].is_background) {
            pipeline->is_background = 1;
        }
        pipeline->num_commands++;
        pipe_str = strtok_r(NULL, "|", &saveptr);
    }

    free(copy);
    return pipeline;
}

void pssim_free_pipeline(pssim_pipeline_t *pipeline) {
    if (!pipeline) return;
    for (int i = 0; i < pipeline->num_commands; i++) {
        pssim_cmd_t *cmd = &pipeline->commands[i];
        if (cmd->argv) {
            for (int j = 0; j < cmd->argc; j++) {
                free(cmd->argv[j]);
            }
            free(cmd->argv);
        }
        if (cmd->input_file) free(cmd->input_file);
        if (cmd->output_file) free(cmd->output_file);
        if (cmd->append_file) free(cmd->append_file);
        if (cmd->error_file) free(cmd->error_file);
    }
    free(pipeline->commands);
    free(pipeline);
}
