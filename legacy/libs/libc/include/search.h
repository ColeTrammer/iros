#ifndef _SEARCH_H
#define _SEARCH_H 1

#include <bits/size_t.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct entry {
    char *key;
    void *data;
} ENTRY;

typedef enum { FIND, ENTER } ACTION;

typedef enum { preorder, postorder, endorder, leaf } VISIT;

int hcreate(size_t size);
void hdestroy(void);
ENTRY *hsearch(ENTRY entry, ACTION action);

void *lfind(const void *key, const void *base, size_t *nmemb, size_t size, int (*compar)(const void *a, const void *b));
void *lsearch(const void *key, const void *base, size_t *nmemb, size_t size, int (*compar)(const void *a, const void *b));

void insque(void *elem, void *prev);
void remque(void *elem);

void *tfind(const void *key, void **rootp, int (*compar)(const void *a, const void *b));
void *tsearch(const void *key, void **rootp, int (*compar)(const void *a, const void *b));
void *tdelete(const void *__restrict key, void **__restrict rootp, int (*compar)(const void *a, const void *b));
void twalk(const void *root, void (*action)(const void *nodep, const VISIT which, const int depth));

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SEARCH_H */
