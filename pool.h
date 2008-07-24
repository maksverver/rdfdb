#ifndef POOL_H_INCLUDED
#define POOL_H_INCLUDED

#include <stdlib.h>

typedef struct pool {
    struct pool_block *blocks;
} *pool_t;

void *palloc(pool_t p, size_t size);
void pfree(pool_t p, void *buffer);
void pclear(pool_t p);
char *pstrdup(pool_t p, const char *st);
char *pprintf(pool_t p, const char *fmt, ...);

#endif /* ndef POOL_H_INCLUDED */
