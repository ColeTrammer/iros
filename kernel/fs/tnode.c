#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <kernel/fs/tnode.h>

struct tnode_list *add_tnode(struct tnode_list *list, struct tnode *tnode) {
    struct tnode_list *to_add = malloc(sizeof(struct tnode_list));
    to_add->tnode = tnode;
    to_add->next = list;
    return to_add;
}

struct tnode_list *remove_tnode(struct tnode_list *list, struct tnode *tnode) {
    assert(list != NULL);
    assert(tnode);
    
    struct tnode_list **link = &list;
    while (*link != NULL) {
        if ((*link)->tnode == tnode) {
            *link = (*link)->next;
            break;
        }

        link = &(*link)->next;
    }

    return list;
}

struct tnode *find_tnode(struct tnode_list *list, const char *name) {
    assert(list != NULL);
    assert(name);

    struct tnode_list *start = list;
    while (start != NULL) {
        assert(start->tnode);
        assert(start->tnode->name);

        if (strcmp(name, start->tnode->name) == 0) {
            return start->tnode;
        }

        start = start->next;
    }

    return NULL;
}