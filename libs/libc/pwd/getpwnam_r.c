#include <errno.h>
#include <pwd.h>
#include <stddef.h>
#include <string.h>

int getpwnam_r(const char *name, struct passwd *pwd, char *buf, size_t buflen, struct passwd **result) {
    setpwent();
    for (;;) {
        struct passwd *iter;
        int ret = getpwent_r(pwd, buf, buflen, &iter);
        if (ret) {
            goto fail;
        }

        if (strcmp(iter->pw_name, name) == 0) {
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
