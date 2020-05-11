#include <libgen.h>
#include <string.h>

char *basename(char *path) {
    if (!path || !path[0]) {
        return NULL;
    }

    size_t path_len = strlen(path);
    char *last_slash = strrchr(path, '/');
    if (last_slash - path == (ptrdiff_t)(path_len - 1)) {
        *last_slash = '\0';
        last_slash = strrchr(path, '/');
    }

    if (!last_slash || path == last_slash) {
        return path;
    }

    return last_slash + 1;
}
