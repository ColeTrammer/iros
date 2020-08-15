#include <errno.h>
#include <pwd.h>
#include <stddef.h>
#include <string.h>

int getpwuid_r(uid_t uid, struct passwd *pwd, char *buf, size_t buflen, struct passwd **result) {
    setpwent();
    for (;;) {
        struct passwd *iter;
        int ret = getpwent_r(pwd, buf, buflen, &iter);
        if (ret) {
            goto fail;
        }

        if (iter->pw_uid == uid) {
            *result = iter;
            endpwent();
            return 0;
        }
    }

fail:
    *result = NULL;
    errno = ENOENT;
    endpwent();
    return -1;
}
