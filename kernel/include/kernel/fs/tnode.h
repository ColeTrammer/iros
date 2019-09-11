#ifndef _KERNEL_FS_TNODE_H
#define _KERNEL_FS_TNODE_H 1

#include <kernel/fs/inode.h>

struct tnode {
    char *name;
    struct inode *inode;
};

struct tnode_list {
    struct tnode *tnode;
    struct tnode_list *next;
};

struct tnode_list *add_tnode(struct tnode_list *list, struct tnode *tnode);
struct tnode_list *remove_tnode(struct tnode_list *list, struct tnode *tnode);

#endif /* _KERNEL_FS_TNODE_H */