#include "linkedlist.h"

#define NEXT(data) (*(void**)(data))
#define NEXT(data) (*(void**)(data))

void *llreverse(void *data)
{
    void *tail, *p;

    tail = NULL;
    while(data != NULL)
    {
        p = NEXT(data);
        NEXT(data) = tail;
        tail = data;
        data = p;
    }
    return tail;
}

size_t llsize(void *data)
{
    size_t s = 0;
    while(data)
    {
        data = NEXT(data);
        ++s;
    }
    return s;
}

void *llappend(void *data1, void *data2)
{
    void *p;

    if(data1 == NULL)
        return data2;

    for(p = data1; NEXT(p); p = NEXT(p)) { };
    NEXT(p) = data2;
    return data1;
}


void *llsort(llcmp_t cmp, void *data)
{
    void *p, *q;

    if(data == NULL || NEXT(data) == NULL)
        return data;

    /* Find end and half of list */
    q = data;
    p = NEXT(data);
    while(p != NULL && NEXT(p) != NULL) {
        q = NEXT(q);
        p = NEXT(NEXT(p));
    }

    /* Split at q - halfway */
    p = NEXT(q);
    NEXT(q) = NULL;

    /* Divide and conquer! */
    llsort(cmp, data);
    llsort(cmp, p);
    return llmerge(cmp, data, p);
}

void *lluniq(llcmp_t cmp, void *data, void **dups)
{
    void *first, *last, *dups_first, *dups_last;

    if(data == NULL)
    {
        if(dups)
            *dups = NULL;
        return NULL;
    }

    first = last = data;
    dups_first = dups_last = NULL;

    for(data = NEXT(data); data != NULL; data = NEXT(data))
    {
        if(cmp(last, data) == 0)
        {
            /* Duplicate found */
            if(dups_first == NULL)
            {
                dups_first = dups_last = data;
            }
            else
            {
                NEXT(dups_last) = data;
                dups_last       = data;
                data            = NEXT(data);
            }
        }
        else
        {
            /* Unique element found */
            NEXT(last) = data;
            last       = data;
        }
    }

    if(dups != NULL)
    {
        if(dups_last != NULL)
            NEXT(dups_last) = NULL;

        *dups = llreverse(dups_first);
    }

    NEXT(last) = NULL;
    return first;
}

void *llmerge(llcmp_t cmp, void *data1, void *data2)
{
    void *first, *last;

    if(data1 == NULL)
        return data2;

    if(data2 == NULL)
        return data1;

    /* Determine first element */
    if(cmp(data1, data2) <= 0)
    {
        first = last = data1;
        data1 = NEXT(data1);
    }
    else
    {
        first = last = data2;
        data2 = NEXT(data2);
    }

    /* Merge remaining elements */
    while(data1 && data2)
    {
        if(cmp(data1, data2) <= 0)
        {
            NEXT(last) = data1;
            last       = data1;
            data1      = NEXT(data1);
        }
        else
        {
            NEXT(last) = data2;
            last       = data2;
            data2      = NEXT(data2);
        }
    }

    if(data1 == NULL)
        NEXT(last) = data2;
    else
        NEXT(last) = data1;

    return first;
}


/* Some test code follows... */

#include <stdio.h>

struct test
{
    struct test *next;

    int n;
};

void test_print(struct test *data)
{
    for( ; data != NULL; data = data->next)
        printf("%d ", data->n);

    printf("\n");
}

int test_cmp(const struct test *a, const struct test *b)
{
    return (a->n/2) - (b->n/2);
}

int main()
{
    struct test elems[] = {
        { &elems[1],  4 },
        { &elems[2], 10 },
        { &elems[3],  5 },
        { &elems[4], -6 },
        { &elems[5],  1 },
        {     NULL, 12 } };
    struct test *data = elems, *dups;

    printf("Data:            ");
    test_print(data);

    data = llreverse(data);
    printf("Reversed:        ");
    test_print(data);

    data = llreverse(data);
    printf("Twice reversed:  ");
    test_print(data);

    data = llsort((llcmp_t)test_cmp, data);
    printf("Sorted:          ");
    test_print(data);

    data = lluniq((llcmp_t)test_cmp, data, (void**)&dups);
    printf("Unique numbers:  ");
    test_print(data);
    printf("Duplicates:      ");
    test_print(dups);

    return 0;
}
