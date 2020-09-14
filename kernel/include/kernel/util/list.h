#ifndef _KERNEL_UTIL_LIST_H
#define _KERNEL_UTIL_LIST_H 1

#include <kernel/util/macros.h>

struct list_node {
    struct list_node *next;
    struct list_node *prev;
};

#define INIT_LIST(l) \
    (struct list_node) { .prev = &(l), .next = &(l) }

static inline void init_list(struct list_node *head) {
    head->next = head->prev = head;
}

static inline void list_append(struct list_node *head, struct list_node *elem) {
    elem->next = head;
    elem->prev = head->prev;
    head->prev->next = elem;
    head->prev = elem;
}

static inline void list_prepend(struct list_node *head, struct list_node *elem) {
    elem->next = head->next;
    elem->prev = head;
    head->next->prev = elem;
    head->next = elem;
}

static inline void list_remove(struct list_node *elem) {
    elem->prev->next = elem->next;
    elem->next->prev = elem->prev;
    elem->prev = elem->next = elem;
}

static inline bool list_is_empty(struct list_node *head) {
    return head->next == head;
}

static inline bool list_is_singular(struct list_node *head) {
    return !list_is_empty(head) && head->next == head->prev;
}

static inline void list_clear(struct list_node *head) {
    init_list(head);
}

#define list_entry container_of

#define list_for_each_entry(head, iter, type, member)                                    \
    for (type *iter = container_of((head)->next, type, member); &iter->member != (head); \
         iter = container_of(iter->member.next, type, member))

#define list_for_each_entry_safe(head, iter, type, member)                                                               \
    for (type *iter = container_of((head)->next, type, member), *__next = container_of(iter->member.next, type, member); \
         &iter->member != (head); iter = __next, __next = container_of(iter->member.next, type, member))

#define list_first_entry(head, type, member) (list_is_empty(head) ? NULL : container_of((head)->next, type, member))
#define list_last_entry(head, type, member)  (list_is_empty(head) ? NULL : container_of((head)->prev, type, member))

#define list_next_entry(head, node, type, member) ((head) != (node)->next ? list_entry((node)->next, type, member) : NULL)

#endif /* _KERNEL_UTIL_LIST_H */
