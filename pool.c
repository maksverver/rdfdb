#include "pool.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

struct pool_block
{
    struct pool_block *next;
};


void *palloc(pool_t p, size_t size)
{
    struct pool_block *block = (struct pool_block*)malloc(8 + size);

    if(block == NULL)
        return NULL;

    block->next = p->blocks;
    p->blocks   = block;

    return ((char*)block) + 8;
}

void pfree(pool_t p, void *buffer)
{
    struct pool_block *block = p->blocks, *last = NULL;

    buffer = ((char*)buffer) - 8;
    while(block)
    {
        if(block == buffer)
        {
            if(last == NULL)
                p->blocks = block->next;
            else
                last->next = block->next;

            free(buffer);

            return;
        }

        last  = block;
        block = block->next;
    }

    fprintf(stderr, "pfree(): invalid block passed; not freed!\n");
}

void pclear(pool_t p)
{
    struct pool_block *block = p->blocks, *next;

    while(block)
    {
        next = block->next;
        free(block);
        block = next;
    }
}

char *pstrdup(pool_t p, const char *str)
{
    char *buf;
    int size = strlen(str) + 1;
    if((buf = (char*)palloc(p, size)))
        memcpy(buf, str, size);
    return buf;
}

char *pprintf(pool_t p, const char *fmt, ...)
{
    int vsnprintf(char *str, size_t size, const char *format, va_list ap);

    va_list ap;
    char *buf;
    int buflen;

    va_start(ap, fmt);
    buflen = 1 + vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    if((buf = (char*)palloc(p, buflen)))
    {
        va_start(ap, fmt);
        vsnprintf(buf, buflen, fmt, ap);
        va_end(ap);
    }

    return buf;
}
