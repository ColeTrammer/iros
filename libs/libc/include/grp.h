#ifndef _GRP_H
#define _GRP_H 1

#include <bits/gid_t.h>
#include <bits/size_t.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct group {
    char *gr_name;
    char *gr_passwd;
    gid_t gr_gid;
    char **gr_mem;
};

struct group *getgrgid(gid_t gid);
struct group *getgrnam(const char *name);

int getgrgid_r(gid_t gid, struct group *group, char *buf, size_t buf_size, struct group **result);
int getgrnam_r(const char *name, struct group *group, char *buf, size_t buf_size, struct group **result);

void endgrent(void);
void setgrent(void);
struct group *getgrent(void);

int setgroups(size_t size, const gid_t *list);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _GRP_H */
