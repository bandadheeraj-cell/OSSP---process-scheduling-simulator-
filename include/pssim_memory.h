#ifndef PSSIM_MEMORY_H
#define PSSIM_MEMORY_H

#include <stddef.h>

void *pssim_malloc(size_t size);
void *pssim_calloc(size_t num, size_t size);
void *pssim_realloc(void *ptr, size_t size);
char *pssim_strdup(const char *s);
void pssim_free(void *ptr);

/* Utility to safely free array of dynamic strings */
void pssim_free_tokens(char **tokens);

#endif /* PSSIM_MEMORY_H */
