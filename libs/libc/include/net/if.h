#ifndef _NET_IF_H
#define _NET_IF_H 1

#define IF_NAMESIZE 16

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct if_nameindex {
    unsigned int if_index;
    char *if_name;
};

void if_freenameindex(struct if_nameindex *nameindex);
char *if_indextoname(unsigned int index, char *name);
struct if_nameindex *if_nameindex(void);
unsigned int if_nametoindex(const char *name);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _NET_IF_H */
