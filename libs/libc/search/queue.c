#include <search.h>
#include <stddef.h>
#include <string.h>

struct queue_ent {
    struct queue_ent *next;
    struct queue_ent *prev;
};

void insque(void *elem, void *prev) {
    struct queue_ent *to_add = elem;
    struct queue_ent *ent = prev;

    if (prev == NULL) {
        to_add->next = NULL;
        to_add->prev = NULL;
        return;
    }

    // ent is prev
    to_add->next = ent->next;
    to_add->prev = ent;

    if (to_add->next) {
        to_add->next->prev = to_add;
    }
    ent->next = to_add;
}

void remque(void *elem) {
    struct queue_ent *to_remove = elem;

    struct queue_ent *prev = to_remove->prev;
    struct queue_ent *next = to_remove->next;

    if (prev) {
        prev->next = to_remove->next;
    }

    if (next) {
        next->prev = to_remove->prev;
    }

    to_remove->next = NULL;
    to_remove->prev = NULL;
}