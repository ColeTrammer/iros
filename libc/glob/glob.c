#include <glob.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>

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

static bool glob_append(glob_t *pglob, char *s) {
    if (pglob->gl_pathc == 0) {
        pglob->gl_pathv = calloc(10, sizeof(char*));
    } else if (pglob->gl_pathc % GLOB_BUF_INCREMENT == 0) {
        pglob->gl_pathv = realloc(pglob->gl_pathv, pglob->gl_pathc + GLOB_BUF_INCREMENT * sizeof(char*));
    }

    // Memory allocation error
    if (pglob->gl_pathv == NULL) {
        return false;
    }

    pglob->gl_pathv[pglob->gl_pathc] = malloc(strlen(s) + 1);
    strcpy(pglob->gl_pathv[pglob->gl_pathc++], s);
    return true;
}

static bool glob_empty(glob_t *pglob) {
    return pglob->gl_pathc == 0;
}

int glob(const char *__restrict pattern, int flags, int (*errfunc)(const char *epath, int eerrno), glob_t *__restrict pglob) {
    assert(!(flags & GLOB_MARK));
    assert(!(flags & GLOB_DOOFFS));

    if (!(flags & GLOB_APPEND)) {
        memset(pglob, 0, sizeof(glob_t));
    }

    char *first_slash = strchr(pattern, '/');
    assert(first_slash == NULL);

    DIR *d = opendir("./");
    if (!d) {
        int ret = 0;
        if (errfunc) {
            ret = errfunc("./", errno);
        }
        if (flags & GLOB_ERR) {
            ret = 1;
        }
        if (ret != 0) {
            return GLOB_ABORTED;
        }
    }

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (glob_matches(ent->d_name, pattern)) {
            if (!glob_append(pglob, ent->d_name)) {
                return GLOB_NOSPACE;
            }
        }
    }

    return glob_empty(pglob) ? GLOB_NOMATCH : 0;
}

void globfree(glob_t *pglob) {
    for (size_t i = pglob->gl_offs; i < pglob->gl_pathc; i++) {
        if (pglob->gl_pathv[i] != NULL) {
            free(pglob->gl_pathv[i]);
        }
    }

    free(pglob->gl_pathv);
}