#include <errno.h>
#include <grp.h>
#include <stddef.h>

int getgrgid_r(gid_t gid, struct group *group, char *buf, size_t buf_size, struct group **result) {
    setgrent();
    for (;;) {
        struct group *iter;
        int ret = getgrent_r(group, buf, buf_size, &iter);
        if (ret) {
            goto fail;
        }

        if (iter->gr_gid == gid) {
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
