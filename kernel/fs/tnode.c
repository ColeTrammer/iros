#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

#include <kernel/fs/tnode.h>

struct tnode_list *add_tnode(struct tnode_list *list, struct tnode *tnode) {
    struct tnode_list *to_add = malloc(sizeof(struct tnode_list));
    to_add->tnode = tnode;
    to_add->next = list;
    return to_add;
}

struct tnode_list *remove_tnode(struct tnode_list *list, struct tnode *tnode) {
    assert(list != NULL);
    
    struct tnode_list **link = &list;
    while (*link != NULL) {
        if ((*link)->tnode == tnode) {
            *link = (*link)->next;
        }

        link = &(*link)->next;
    }

    return list;
}