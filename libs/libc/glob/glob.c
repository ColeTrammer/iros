#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <glob.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define GLOB_BUF_INCREMENT 10

// Determines whether a character is in a given set
static bool glob_is_valid_char_for_set(char c, const char *set, int set_end, bool invert) {
    for (int i = 0; i < set_end; i++) {
        // Handle `-` ranges
        if (i != 0 && i != set_end - 1 && set[i] == '-') {
            char range_start = set[i - 1];
            char range_end = set[i + 1];

            // Switch ranges so they work correctly if specified backwards
            if (range_start > range_end) {
                char t = range_end;
                range_end = range_start;
                range_start = t;
            }

            // Don't need to check edges b/c they are checked automatically
            for (char r = range_start + 1; r < range_end; r++) {
                if (r == c) {
                    return !invert;
                }
            }
        }

        if (set[i] == c) {
            return !invert;
        }
    }

    return invert;
}

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
            pi++;
            bool invert = false;
            if (pattern[pi] == '!') {
                invert = true;
                pi++;
            }
            // Sets cannot be empty
            pi++;
            const char *set_start = pattern + pi;
            while (pattern[pi] != ']') {
                pi++;
            }
            if (!glob_is_valid_char_for_set(s[si++], set_start, pi++, invert)) {
                // Try again if we were trying to match a `*`
                if (next_si != 0) {
                    si = next_si;
                    pi = next_pi;
                } else {
                    return false;
                }
            }
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
        if (glob_matches(ent->d_name, pattern)) {
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