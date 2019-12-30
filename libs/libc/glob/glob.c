#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fnmatch.h>
#include <glob.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define GLOB_BUF_INCREMENT 10

static bool glob_append(glob_t *pglob, char *p, char *s) {
    if (pglob->gl_pathc == 0) {
        pglob->gl_pathv = calloc(10, sizeof(char *));
    } else if (pglob->gl_pathc % GLOB_BUF_INCREMENT == 0) {
        pglob->gl_pathv = realloc(pglob->gl_pathv, (pglob->gl_pathc + GLOB_BUF_INCREMENT) * sizeof(char *));
    }

    // Memory allocation error
    if (pglob->gl_pathv == NULL) {
        return false;
    }

    pglob->gl_pathv[pglob->gl_pathc] = malloc(strlen(p) + strlen(s) + 1);
    if (pglob->gl_pathv[pglob->gl_pathc] == NULL) {
        return false;
    }

    strcpy(pglob->gl_pathv[pglob->gl_pathc], p);
    strcat(pglob->gl_pathv[pglob->gl_pathc++], s);
    return true;
}

static bool glob_is_dir(char *path, int flags, int (*errfunc)(const char *epath, int eerno), bool *is_dir) {
    struct stat stat_struct;
    if (stat(path, &stat_struct) != 0) {
        int ret = 0;
        if (errfunc) {
            ret = errfunc(path, errno);
        }
        if (flags & GLOB_ERR) {
            ret = 1;
        }
        if (ret) {
            return false;
        }
    }
    *is_dir = S_ISDIR(stat_struct.st_mode);
    return true;
}

static bool glob_opendir(char *path, int flags, int (*errfunc)(const char *epath, int eerrno), DIR **d) {
    assert(d);

    *d = opendir(path);
    if (!*d) {
        int ret = 0;
        if (errfunc) {
            ret = errfunc(path, errno);
        }
        if (flags & GLOB_ERR) {
            ret = 1;
        }
        if (ret != 0) {
            return false;
        }
    }

    return true;
}

static int glob_helper(char *__restrict path, char *__restrict to_prepend, const char *__restrict pattern, int flags,
                       int (*errfunc)(const char *epath, int eerrno), glob_t *__restrict pglob) {
    char *first_slash = strchr(pattern, '/');

    if (first_slash) {
        *first_slash = '\0';
    }

    DIR *d;
    if (!glob_opendir(path, flags, errfunc, &d)) {
        return GLOB_ABORTED;
    }

    struct dirent *ent;
    bool found_anything = false;
    while ((ent = readdir(d)) != NULL) {
        int ret = fnmatch(pattern, ent->d_name, FNM_PERIOD | ((flags & GLOB_NOESCAPE) ? FNM_NOESCAPE : 0));
        if (ret != 0 && ret != FNM_NOMATCH) {
            return GLOB_ERR;
        }

        if (ret == 0) {
            if (first_slash == NULL) {
                found_anything = true;
                if (!glob_append(pglob, to_prepend, ent->d_name)) {
                    closedir(d);
                    return GLOB_NOSPACE;
                }
            } else {
                char *new_path = malloc(strlen(path) + strlen(ent->d_name) + 2);
                strcpy(new_path, path);
                strcat(new_path, "/");
                strcat(new_path, ent->d_name);

                char *new_to_prepend = malloc(strlen(to_prepend) + strlen(ent->d_name) + 2);
                strcpy(new_to_prepend, to_prepend);
                strcat(new_to_prepend, ent->d_name);
                strcat(new_to_prepend, "/");

                bool is_dir;
                if (!glob_is_dir(new_path, flags, errfunc, &is_dir)) {
                    *first_slash = '/';
                    free(new_path);
                    free(new_to_prepend);
                    closedir(d);
                    return GLOB_ABORTED;
                }

                if (!is_dir) {
                    continue;
                }

                int ret = glob_helper(new_path, new_to_prepend, first_slash + 1, flags, errfunc, pglob);
                free(new_path);
                free(new_to_prepend);
                if ((ret != 0 && ret != GLOB_NOMATCH) && (flags & GLOB_ERR)) {
                    *first_slash = '/';
                    closedir(d);
                    return ret;
                }
                if (ret != GLOB_NOMATCH) {
                    found_anything = true;
                }
            }
        }
    }

    closedir(d);
    if (first_slash) {
        *first_slash = '/';
    }
    return found_anything ? 0 : GLOB_NOMATCH;
}

static int glob_compare(const void *s1, const void *s2) {
    return strcmp(*(char *const *) s1, *(char *const *) s2);
}

int glob(const char *__restrict pattern, int flags, int (*errfunc)(const char *epath, int eerrno), glob_t *__restrict pglob) {
    assert(!(flags & GLOB_MARK));

    if (!(flags & GLOB_APPEND)) {
        size_t offs_save = pglob->gl_offs;
        memset(pglob, 0, sizeof(glob_t));
        pglob->gl_offs = (flags & GLOB_DOOFFS) ? offs_save : 0;
    }

    if (flags & GLOB_DOOFFS) {
        pglob->gl_pathc = pglob->gl_offs;
        pglob->gl_pathv = calloc(GLOB_BUF_INCREMENT * ((pglob->gl_offs + GLOB_BUF_INCREMENT - 1) / GLOB_BUF_INCREMENT), sizeof(char *));
    }

    int ret;
    if (pattern[0] == '/') {
        ret = glob_helper("/.", "", pattern + 1, flags, errfunc, pglob);
    } else {
        ret = glob_helper(".", "", pattern, flags, errfunc, pglob);
    }

    if (ret != 0) {
        return ret;
    }

    if (!(flags & GLOB_NOSORT)) {
        qsort(pglob->gl_pathv + pglob->gl_offs, pglob->gl_pathc - pglob->gl_offs, sizeof(char *), glob_compare);
    }

    return ret;
}

void globfree(glob_t *pglob) {
    for (size_t i = pglob->gl_offs; i < pglob->gl_pathc; i++) {
        if (pglob->gl_pathv[i] != NULL) {
            free(pglob->gl_pathv[i]);
        }
    }

    free(pglob->gl_pathv);
}