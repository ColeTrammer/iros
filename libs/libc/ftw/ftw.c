#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <ftw.h>
#include <search.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

typedef int (*fn_t)(const char *path, const struct stat *stat_struct, int type, struct FTW *info);

struct ftw_dirent {
    struct ftw_dirent *next;
    struct ftw_dirent *prev;
    char *path;
    bool expanded;
    struct stat stat_struct;
    int flag;
    int depth;
};

static struct ftw_dirent *start = NULL;
static struct ftw_dirent *end = NULL;
static size_t size = 0;

static void free_dirent(struct ftw_dirent *d) {
    free(d->path);
    free(d);
}

static int add_dirent(char *path, int depth) {
    assert(path);

    struct ftw_dirent *d = malloc(sizeof(struct ftw_dirent));
    if (!d) {
        return -1;
    }

    d->path = path;
    d->expanded = false;
    d->depth = depth;
    if (!end || !start) {
        start = end = d;
        insque(d, NULL);
    } else {
        insque(d, end);
        end = d;
    }

    size++;
    return 0;
}

static struct ftw_dirent *consume_dirent() {
    assert(size > 0);
    assert(start);

    struct ftw_dirent *d = start;
    start = start->next;
    if (!start) {
        end = NULL;
    }
    remque(d);
    size--;

    return d;
}

static struct ftw_dirent *pop_dirent() {
    assert(size > 0);
    assert(end);

    struct ftw_dirent *d = end;
    end = end->prev;
    if (!end) {
        start = NULL;
    }
    remque(d);
    size--;

    return d;
}

static struct ftw_dirent *peek_dirent() {
    assert(end);
    return end;
}

static int ftw_call(fn_t fn, struct ftw_dirent *dirent) {
    struct FTW ftw_buf;
    char *last_slash = strrchr(dirent->path, '/');
    ftw_buf.base = last_slash ? last_slash - dirent->path + 1 : 0;
    ftw_buf.level = dirent->depth;

    return fn(dirent->path, &dirent->stat_struct, dirent->flag, &ftw_buf);
}

static int ftw_build_dirents(const char *path, int depth) {
    DIR *d = opendir(path);
    if (!d) {
        return -1;
    }

    struct dirent *ent;
    while ((ent = readdir(d))) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }

        char *new_path = malloc(strlen(path) + strlen(ent->d_name) + 2);
        strcpy(new_path, path);
        strcat(new_path, "/");
        strcat(new_path, ent->d_name);
        add_dirent(new_path, depth);
    }

    return closedir(d);
}

static int ftw_expand(struct ftw_dirent *d, int flags) {
    int ret = 0;
    if (!(flags & FTW_PHYS ? lstat : stat)(d->path, &d->stat_struct)) {
        if (S_ISDIR(d->stat_struct.st_mode)) {
            if (access(d->path, R_OK)) {
                d->flag = FTW_DNR;
            } else {
                d->flag = flags & FTW_DEPTH ? FTW_DP : FTW_D;
                ret = ftw_build_dirents(d->path, d->depth + 1);
            }
        } else if (S_ISLNK(d->stat_struct.st_mode)) {
            d->flag = FTW_SL;
        } else {
            d->flag = FTW_F;
        }
    } else {
        d->flag = FTW_NS;
    }

    return ret;
}

static int ftw_traverse_bf(fn_t fn, int flags) {
    int depth = 0;
    while (size > 0) {
        size_t size_save = size;
        for (size_t i = 0; i < size_save; i++) {
            struct ftw_dirent *d = consume_dirent();
            ftw_expand(d, flags);

            int ret = ftw_call(fn, d);

            free_dirent(d);

            if (ret) {
                return ret;
            }
        }

        depth++;
    }

    return 0;
}

static int ftw_traverse_df(fn_t fn, int flags) {
    while (size > 0) {
        struct ftw_dirent *d = peek_dirent();

        if (!d->expanded) {
            d->expanded = true;
            ftw_expand(d, flags);
            continue;
        }

        pop_dirent();
        int ret = ftw_call(fn, d);

        free_dirent(d);

        if (ret) {
            return ret;
        }
    }

    return 0;
}

int nftw(const char *path, fn_t fn, int fd_limit, int flags) {
    if (fd_limit <= 0) {
        return -1;
    }

    assert(!(flags & FTW_CHDIR));

    char *last_slash = strrchr(path, '/');
    char *path_copy = strdup(path);
    if (last_slash == path + strlen(path) - 1) {
        path_copy[strlen(path) - 1] = '\0';
    }

    int ret = add_dirent(path_copy, 0);
    if (ret != 0) {
        goto cleanup;
    }

    if (flags & FTW_DEPTH) {
        ret = ftw_traverse_df(fn, flags);
    } else {
        ret = ftw_traverse_bf(fn, flags);
    }

cleanup:
    while (size > 0) {
        free(consume_dirent());
    }

    return ret;
}

static int (*ftw_current_fn)(const char *path, const struct stat *stat_struct, int type);

int ftw_fn_wrapper(const char *path, const struct stat *stat_struct, int type, struct FTW *ftw) {
    (void) ftw;
    return ftw_current_fn(path, stat_struct, type);
}

int ftw(const char *path, int (*fn)(const char *path, const struct stat *stat_struct, int type), int fd_limit) {
    ftw_current_fn = fn;
    return nftw(path, &ftw_fn_wrapper, fd_limit, 0);
}