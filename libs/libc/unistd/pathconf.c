#include <errno.h>
#include <limits.h>
#include <unistd.h>

long pathconf(const char *path, int name) {
    (void) path;
    switch (name) {
        case _PC_2_SYMLINKS:
            return 1;
        case _PC_ALLOC_SIZE_MIN:
            return -1;
        case _PC_ASYNC_IO:
            return -1;
        case _PC_CHOWN_RESTRICTED:
            return _POSIX_CHOWN_RESTRICTED;
        case _PC_FILESIZEBITS:
            return FILESIZEBITS;
        case _PC_LINK_MAX:
            return LINK_MAX;
        case _PC_MAX_CANON:
            return PIPE_BUF;
        case _PC_MAX_INPUT:
            return PIPE_BUF;
        case _PC_NAME_MAX:
            return NAME_MAX;
        case _PC_NO_TRUNC:
            return 1;
        case _PC_PATH_MAX:
            return PATH_MAX;
        case _PC_PIPE_BUF:
            return PIPE_BUF;
        case _PC_PRIO_IO:
            return -1;
        case _PC_REC_INCR_XFER_SIZE:
            return -1;
        case _PC_REC_MAX_XFER_SIZE:
            return -1;
        case _PC_REC_MIN_XFER_SIZE:
            return -1;
        case _PC_REC_XFER_ALIGN:
            return -1;
        case _PC_SYMLINK_MAX:
            return 255;
        case _PC_SYNC_IO:
            return -1;
        case _PC_TIMESTAMP_RESOLUTION:
            return 1000;
        case _PC_VDISABLE:
            return _POSIX_VDISABLE;
        default:
            errno = EINVAL;
            return -1;
    }
}
