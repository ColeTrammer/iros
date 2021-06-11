#ifndef _KERNEL_FS_TNODE_H
#define _KERNEL_FS_TNODE_H 1

#include <kernel/fs/inode.h>

struct mount;

// NOTE: tnodes are immutable, so there is no need for any locks
struct tnode {
    char *name;
    struct inode *inode;
    struct tnode *parent;
    struct mount *mount;
    int ref_count;
};

struct tnode *create_root_tnode(struct inode *inode, struct mount *root);
struct tnode *create_tnode_from_characters(const char *characters, size_t length, struct tnode *parent, struct inode *inode,
                                           struct mount *mount);
struct tnode *create_tnode(const char *name_to_copy, struct tnode *tnode, struct inode *inode, struct mount *mount);
void drop_tnode(struct tnode *tnode);
struct tnode *bump_tnode(struct tnode *tnode);

#endif /* _KERNEL_FS_TNODE_H */
