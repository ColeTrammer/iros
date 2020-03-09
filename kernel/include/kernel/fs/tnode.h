#ifndef _KERNEL_FS_TNODE_H
#define _KERNEL_FS_TNODE_H 1

#include <kernel/fs/inode.h>
#include <kernel/util/spinlock.h>

struct tnode {
    char *name;
    struct inode *inode;
    spinlock_t lock;
    int ref_count;
};

struct tnode_list {
    struct tnode *tnode;
    struct tnode_list *next;
};

struct tnode *create_root_tnode(struct inode *inode);
struct tnode *create_tnode_from_characters(const char *characters, size_t length, struct inode *inode);
struct tnode *create_tnode(const char *name_to_copy, struct inode *inode);
void drop_tnode(struct tnode *tnode);
struct tnode *bump_tnode(struct tnode *tnode);

struct tnode_list *add_tnode(struct tnode_list *list, struct tnode *tnode);
struct tnode_list *add_tnode_before(struct tnode_list *list, struct tnode_list *after, struct tnode *tnode);
struct tnode_list *remove_tnode(struct tnode_list *list, struct tnode *tnode);
struct tnode *find_tnode(struct tnode_list *list, const char *name);
struct tnode *find_tnode_inode(struct tnode_list *list, struct inode *inode);
struct tnode *find_tnode_index(struct tnode_list *list, size_t index);
size_t get_tnode_list_length(struct tnode_list *list);
void free_tnode_list_and_tnodes(struct tnode_list *list);

#endif /* _KERNEL_FS_TNODE_H */