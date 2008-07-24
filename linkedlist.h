#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <stdlib.h>

/* A comparator function; returns <0 if the first argument is smaller
   than the second, >0 if the first is larger than the second, and 0
   if the elements are equal. */
typedef int(*llcmp_t)(const void *, const void *);

/* Returns the number of items in 'data' in O(N) time. */
size_t llsize(void *data);

/* Reverses 'data' and returns a pointer to the new beginning of the list,
   in O(N) time. */
void *llreverse(void *data);

/* Appends data2 at the end of data1 and returns a pointer to the beginning
   of the list, in O(N) time (where N is the length of the resulting list). */
void *llappend(void *data1, void *data2);

/* Sorts 'data' in O(N log N) time using a stable sorting algorithm and 'cmp'
   as the comparator. A pointer to the beginning of the sorted list is returned. */
void *llsort(llcmp_t cmp, void *data);

/* Removes all consecutive occurences of elements in data except the first
   in O(N) time. This function is usually used on a sorted list.

   A pointer to the beginning of the list is returned, while a pointer to a
   list of the duplicates (removed from data) is put in *dups if dups is
   not NULL. */
void *lluniq(llcmp_t cmp, void *data, void **dups);

/* Merges two sorted lists ('data1' and 'data2') into a single sorted list, using
   the comparator 'cmp'.  A pointer to the beginning of the list is returned.
   For each pair of elements from data1 and data2 which compare equal, the
   element occuring in data1 is put first. */
void *llmerge(llcmp_t cmp, void *data1, void *data2);


#endif /* ndef LINKEDLIST_H */
