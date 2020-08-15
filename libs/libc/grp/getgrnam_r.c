#include <errno.h>
#include <grp.h>
#include <stddef.h>
#include <string.h>

int getgrnam_r(const char *name, struct group *group, char *buf, size_t buf_size, struct group **result) {
    setgrent();
    for (;;) {
        struct group *iter;
        int ret = getgrent_r(group, buf, buf_size, &iter);
        if (ret) {
            goto fail;
        }

        if (strcmp(iter->gr_name, name) == 0) {
            *result = iter;
            endgrent();
            return 0;
        }
    }

fail:
    *result = NULL;
    errno = ENOENT;
    endgrent();
    return -1;
}
