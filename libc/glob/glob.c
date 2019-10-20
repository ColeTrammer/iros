#include <glob.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/stat.h>

#define GLOB_BUF_INCREMENT 10

static bool glob_matches(char *s, const char *pattern) {
    size_t si = 0;
    size_t pi = 0;

    // Avoid recursion by using greedy matching (https://research.swtch.com/glob)
    size_t next_si = 0;
    size_t next_pi = 0;
    while (s[si] != '\0' && pattern[pi] != '\0') {
        if (pattern[pi] == '*') {
            // We're not allowed to match dots
            if (si == 0 && s[si] == '.') {
                return false;
            }

            // Start by matching nothing, but we will continually try again with higher values of si until it matches
            next_pi = pi;
            next_si = si + 1;
            pi++;

            // If the last in the pattern character is a `*` - simply return true
            if (pattern[pi] == '\0') {
                return true;
            }
        } else if (pattern[pi] == '?') {
            if (si == 0 && s[si] == '.') {
                return false;
            }
            si++;
            pi++;
        } else if (pattern[pi] == '[') {
            assert(false);
        } else {
            if (s[si++] != pattern[pi++]) {
                // Try again if we were trying to match a `*`
                if (next_si != 0) {
                    si = next_si;
                    pi = next_pi;
                } else {
                    return false;
                }
            }
        }
    }

    return s[si] == '\0' && pattern[pi] == '\0';
}

static bool glob_append(glob_t *pglob, char *p, char *s) {
    if (pglob->gl_pathc == 0) {
        pglob->gl_pathv = calloc(10, sizeof(char*));
    } else if (pglob->gl_pathc % GLOB_BUF_INCREMENT == 0) {
        pglob->gl_pathv = realloc(pglob->gl_pathv, (pglob->gl_pathc + GLOB_BUF_INCREMENT) * sizeof(char*));
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

static bool glob_empty(glob_t *pglob) {
    return pglob->gl_pathc == 0;
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

static int glob_helper(char *__restrict path, char *__restrict to_prepend, const char *__restrict pattern, int flags, int (*errfunc)(const char *epath, int eerrno), glob_t *__restrict pglob) {
    char *first_slash = strchr(pattern, '/');

    if (first_slash) {
        *first_slash = '\0';
    }

    DIR *d;
    if (!glob_opendir(path, flags, errfunc, &d)) {
        return GLOB_ABORTED;
    }

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (glob_matches(ent->d_name, pattern)) {
            if (first_slash == NULL) {
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
                if ((ret != 0 && ret != GLOB_NOMATCH) || flags & GLOB_ERR) {
                    *first_slash = '/';
                    closedir(d);
                    return GLOB_ABORTED;
                }
            }
        }
    }

    closedir(d);
    if (first_slash) {
        *first_slash  = '/';
    }
    return glob_empty(pglob) ? GLOB_NOMATCH : 0;
}

int glob(const char *__restrict pattern, int flags, int (*errfunc)(const char *epath, int eerrno), glob_t *__restrict pglob) {
    assert(!(flags & GLOB_MARK));
    assert(!(flags & GLOB_DOOFFS));

    if (!(flags & GLOB_APPEND)) {
        memset(pglob, 0, sizeof(glob_t));
    }

    if (pattern[0] == '/') {
        return glob_helper("/.", "", pattern + 1, flags, errfunc, pglob);
    } else {
        return glob_helper(".", "", pattern, flags, errfunc, pglob);
    }
}

void globfree(glob_t *pglob) {
    for (size_t i = pglob->gl_offs; i < pglob->gl_pathc; i++) {
        if (pglob->gl_pathv[i] != NULL) {
            free(pglob->gl_pathv[i]);
        }
    }

    free(pglob->gl_pathv);
}