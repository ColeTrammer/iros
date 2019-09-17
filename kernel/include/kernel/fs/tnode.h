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
struct tnode *find_tnode(struct tnode_list *list, const char *name);
struct tnode *find_tnode_inode(struct tnode_list *list, struct inode *inode);
struct tnode *find_tnode_index(struct tnode_list *list, size_t index);
size_t get_tnode_list_length(struct tnode_list *list);

#endif /* _KERNEL_FS_TNODE_H */