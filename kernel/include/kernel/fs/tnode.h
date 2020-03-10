#ifndef _KERNEL_FS_TNODE_H
#define _KERNEL_FS_TNODE_H 1

#include <kernel/fs/inode.h>
#include <kernel/util/spinlock.h>

struct tnode {
    char *name;
    struct inode *inode;
    struct tnode *parent;
    spinlock_t lock;
    int ref_count;
};

struct tnode *create_root_tnode(struct inode *inode);
struct tnode *create_tnode_from_characters(const char *characters, size_t length, struct tnode *parent, struct inode *inode);
struct tnode *create_tnode(const char *name_to_copy, struct tnode *tnode, struct inode *inode);
void drop_tnode(struct tnode *tnode);
struct tnode *bump_tnode(struct tnode *tnode);

#endif /* _KERNEL_FS_TNODE_H */