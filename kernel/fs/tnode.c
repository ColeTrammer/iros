#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/fs/tnode.h>
#include <kernel/fs/vfs.h>

// #define TNODE_REF_COUNT_DEBUG

struct tnode *create_root_tnode(struct inode *inode) {
    assert(inode);

    struct tnode *tnode = malloc(sizeof(struct tnode));
    assert(tnode);

    tnode->inode = inode;
    tnode->name = NULL;
    tnode->ref_count = 1;
    init_spinlock(&tnode->lock);

    return tnode;
}

struct tnode *create_tnode_from_characters(const char *characters, size_t length, struct inode *inode) {
    assert(characters);
    assert(inode);

    struct tnode *tnode = malloc(sizeof(struct tnode));
    tnode->inode = inode;
    tnode->name = malloc(length + 1);
    memcpy(tnode->name, characters, length);
    tnode->name[length] = '\0';
    tnode->ref_count = 1;
    init_spinlock(&tnode->lock);

    return tnode;
}

struct tnode *create_tnode(const char *name_to_copy, struct inode *inode) {
    assert(name_to_copy);
    assert(inode);

    struct tnode *tnode = malloc(sizeof(struct tnode));
    assert(tnode);

    tnode->inode = inode;
    tnode->name = strdup(name_to_copy);
    tnode->ref_count = 1;
    init_spinlock(&tnode->lock);

    return tnode;
}

void drop_tnode(struct tnode *tnode) {
    assert(tnode);

    spin_lock(&tnode->lock);

#ifdef TNODE_REF_COUNT_DEBUG
    debug_log("tnode ref_count: [ %p, %d ]\n", tnode, tnode->ref_count - 1);
#endif /* TNODE_REF_COUNT_DEBUG */

    assert(tnode->ref_count > 0);

    if (--tnode->ref_count == 0) {
        spin_unlock(&tnode->lock);

#ifdef TNODE_REF_COUNT_DEBUG
        debug_log("Destroying tnode: [ %p ]\n", tnode);
#endif /* TNODE_REF_COUNT_DEBUG */

        struct inode *inode = tnode->inode;

        free(tnode->name);
        free(tnode);

        return;
    }

    spin_unlock(&tnode->lock);
}

struct tnode *bump_tnode(struct tnode *tnode) {
    assert(tnode);

    spin_lock(&tnode->lock);

#ifdef TNODE_REF_COUNT_DEBUG
    debug_log("tnode ref_count: [ %p, %d ]\n", tnode, tnode->ref_count + 1);
#endif /* TNODE_REF_COUNT_DEBUG */

    assert(tnode->ref_count > 0);

    tnode->ref_count++;

    spin_unlock(&tnode->lock);
    return tnode;
}

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

struct tnode *find_tnode_inode(struct tnode_list *list, struct inode *inode) {
    assert(list != NULL);

    struct tnode_list *start = list;
    while (start != NULL) {
        assert(start->tnode);
        assert(start->tnode->name);

        if (start->tnode->inode == inode) {
            return start->tnode;
        }

        start = start->next;
    }

    return NULL;
}

struct tnode *find_tnode_index(struct tnode_list *list, size_t index) {
    if (list == NULL) {
        return NULL;
    }

    struct tnode_list *start = list;
    size_t i = 0;
    while (start != NULL) {
        if (i++ == index) {
            return start->tnode;
        }

        start = start->next;
    }

    return NULL;
}

size_t get_tnode_list_length(struct tnode_list *list) {
    if (list == NULL) {
        return 0;
    }

    struct tnode_list *start = list;
    size_t i = 0;
    while (start != NULL) {
        i++;
        start = start->next;
    }

    return i;
}

/*  Called to clean up empty dirs with . and .. entries
    Don't need to drop references since these entries are
    ignored anyway.
 */
void free_tnode_list_and_tnodes(struct tnode_list *list) {
    if (list == NULL) {
        return;
    }

    struct tnode_list *start = list;
    while (start != NULL) {
        struct tnode_list *next = start->next;
        free(start->tnode);
        free(start);
        start = next;
    }
}