#include <errno.h>
#include <string.h>
#include <unistd.h>

size_t confstr(int name, char *buf, size_t len) {
    char *value = NULL;
    size_t value_length = 0;
    switch (name) {
        case _CS_PATH:
            value = "/bin:/usr/bin";
            break;
        case _CS_POSIX_V7_ILP32_OFF32_CFLAGS:
        case _CS_POSIX_V7_ILP32_OFF32_LDFLAGS:
        case _CS_POSIX_V7_ILP32_OFF32_LIBS:
        case _CS_POSIX_V7_ILP32_OFFBIG_CFLAGS:
        case _CS_POSIX_V7_ILP32_OFFBIG_LDFLAGS:
        case _CS_POSIX_V7_ILP32_OFFBIG_LIBS:
        case _CS_POSIX_V7_LP64_OFF64_CFLAGS:
        case _CS_POSIX_V7_LP64_OFF64_LDFLAGS:
        case _CS_POSIX_V7_LP64_OFF64_LIBS:
        case _CS_POSIX_V7_LPBIG_OFFBIG_CFLAGS:
        case _CS_POSIX_V7_LPBIG_OFFBIG_LDFLAGS:
        case _CS_POSIX_V7_LPBIG_OFFBIG_LIBS:
        case _CS_POSIX_V7_THREADS_CFLAGS:
        case _CS_POSIX_V7_THREADS_LDFLAGS:
        case _CS_POSIX_V7_WIDTH_RESTRICTED_ENVS:
        case _CS_V7_ENV:
        case _CS_POSIX_V6_ILP32_OFF32_CFLAGS:
        case _CS_POSIX_V6_ILP32_OFF32_LDFLAGS:
        case _CS_POSIX_V6_ILP32_OFF32_LIBS:
        case _CS_POSIX_V6_ILP32_OFFBIG_CFLAGS:
        case _CS_POSIX_V6_ILP32_OFFBIG_LDFLAGS:
        case _CS_POSIX_V6_ILP32_OFFBIG_LIBS:
        case _CS_POSIX_V6_LP64_OFF64_CFLAGS:
        case _CS_POSIX_V6_LP64_OFF64_LDFLAGS:
        case _CS_POSIX_V6_LP64_OFF64_LIBS:
        case _CS_POSIX_V6_LPBIG_OFFBIG_CFLAGS:
        case _CS_POSIX_V6_LPBIG_OFFBIG_LDFLAGS:
        case _CS_POSIX_V6_LPBIG_OFFBIG_LIBS:
        case _CS_POSIX_V6_WIDTH_RESTRICTED_ENVS:
        case _CS_V6_ENV:
            break;
        default:
            errno = EINVAL;
            return 0;
    }

    if (buf && value) {
        value_length = strlen(value);
        strncpy(buf, value, len);
    }
    return value_length;
}
