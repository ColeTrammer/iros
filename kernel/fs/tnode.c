#include <assert.h>
#include <stdatomic.h>
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
    tnode->name = strdup("/");
    tnode->ref_count = 1;
    bump_inode_reference(inode);

    return tnode;
}

struct tnode *create_tnode(const char *name_to_copy, struct tnode *parent, struct inode *inode) {
    assert(name_to_copy);
    assert(inode);

    struct tnode *tnode = malloc(sizeof(struct tnode));
    assert(tnode);

    tnode->parent = parent;
    tnode->inode = inode;
    bump_inode_reference(inode);
    tnode->name = strdup(name_to_copy);
    tnode->ref_count = 1;

    return tnode;
}

void drop_tnode(struct tnode *tnode) {
    assert(tnode);

    int fetched_ref_count = atomic_fetch_sub(&tnode->ref_count, 1);

#ifdef TNODE_REF_COUNT_DEBUG
    debug_log("-tnode ref_count: [ %p, %d ]\n", tnode, fetched_ref_count - 1);
#endif /* TNODE_REF_COUNT_DEBUG */

    assert(fetched_ref_count > 0);

    if (fetched_ref_count == 1) {
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
}

struct tnode *bump_tnode(struct tnode *tnode) {
    assert(tnode);

    int fetched_ref_count = atomic_fetch_add(&tnode->ref_count, 1);
    (void) fetched_ref_count;

#ifdef TNODE_REF_COUNT_DEBUG
    debug_log("+tnode ref_count: [ %p, %d ]\n", tnode, fetched_ref_count + 1);
#endif /* TNODE_REF_COUNT_DEBUG */

    assert(fetched_ref_count > 0);
    return tnode;
}
