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
    tnode->parent = NULL;
    tnode->name = NULL;
    tnode->ref_count = 1;
    init_spinlock(&tnode->lock);

    return tnode;
}

struct tnode *create_tnode(const char *name_to_copy, struct tnode *parent, struct inode *inode) {
    assert(name_to_copy);
    assert(inode);

    struct tnode *tnode = malloc(sizeof(struct tnode));
    assert(tnode);

    tnode->parent = bump_tnode(parent);
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
        struct tnode *parent = tnode->parent;

        free(tnode->name);
        free(tnode);

        drop_inode_reference(inode);
        if (parent) {
            drop_tnode(parent);
        }
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