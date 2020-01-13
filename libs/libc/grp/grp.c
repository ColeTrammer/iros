#include <grp.h>
#include <stdio.h>

#if 0

struct group *getgrgid(gid_t gid) {

}

struct group *getgrnam(const char *name) {

}

int getgrgid_r(gid_t gid, struct group *group, char *buf, size_t buf_size, struct group **result) {

}

int getgrnam_r(const char *name, struct group *group, char *buf, size_t buf_size, struct group **result) {

}

#endif