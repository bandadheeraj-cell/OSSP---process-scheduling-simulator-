#include "../include/pssim_memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *pssim_malloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr && size > 0) {
        perror("[PSSIM MEMORY ERROR] Dynamic memory allocation failed");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void *pssim_calloc(size_t num, size_t size) {
    void *ptr = calloc(num, size);
    if (!ptr && num > 0 && size > 0) {
        perror("[PSSIM MEMORY ERROR] Dynamic zeroed allocation failed");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void *pssim_realloc(void *ptr, size_t size) {
    void *new_ptr = realloc(ptr, size);
    if (!new_ptr && size > 0) {
        perror("[PSSIM MEMORY ERROR] Dynamic memory reallocation failed");
        exit(EXIT_FAILURE);
    }
    return new_ptr;
}

char *pssim_strdup(const char *s) {
    if (!s) return NULL;
    char *dup = strdup(s);
    if (!dup) {
        perror("[PSSIM MEMORY ERROR] String duplication failed");
        exit(EXIT_FAILURE);
    }
    return dup;
}

void pssim_free(void *ptr) {
    if (ptr) {
        free(ptr);
    }
}

void pssim_free_tokens(char **tokens) {
    if (!tokens) return;
    for (int i = 0; tokens[i] != NULL; i++) {
        free(tokens[i]);
    }
    free(tokens);
}
