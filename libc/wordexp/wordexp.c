#include <wordexp.h>
#include <stdlib.h>
#include <glob.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define WE_BUF_INCREMENT 20

static bool we_add(char *s, wordexp_t *we) {
    if (we->we_wordc == 0) {
        we->we_wordv = calloc(WE_BUF_INCREMENT, sizeof(char*));
    } else if (we->we_wordc % WE_BUF_INCREMENT == 0) {
        we->we_wordv = realloc(we->we_wordv, (we->we_wordc + WE_BUF_INCREMENT) * sizeof(char*));
    }

    // Memory allocation error
    if (we->we_wordv == NULL) {
        return false;
    }

    we->we_wordv[we->we_wordc++] = s;
    return true;
}

static int we_split(char *s, char *split_on, wordexp_t *we) {
    bool prev_was_blackslash = false;
    bool in_s_quotes = false;
    bool in_d_quotes = false;

    int prev = strspn(s, split_on);
    for (size_t i = prev + 1;; i++) {
        switch (s[i]) {
            case '\\':
                if (prev_was_blackslash) {
                    prev_was_blackslash = false;
                    break;
                }
                prev_was_blackslash = true;
                continue;
            case '\'':
                in_s_quotes = prev_was_blackslash ? in_s_quotes : !in_s_quotes;
                break;
            case '"':
                in_d_quotes = prev_was_blackslash ? in_d_quotes : !in_d_quotes;
                break;
            default:
                break;
        }

        if (prev_was_blackslash || in_s_quotes || in_d_quotes) {
            if (s[i] == '\0') {
                return WRDE_SYNTAX;
            }
            prev_was_blackslash = false;
            continue;
        }

        size_t to_advance = 0;
        if (s[i] != '\0' && (to_advance = strspn(s + i, split_on)) == 0) {
            continue;
        }

        size_t stopping_point = i;
        char *to_add = malloc(stopping_point - prev + 1);
        memcpy(to_add, s + prev, stopping_point - prev);
        to_add[stopping_point - prev] = '\0';

        if (!we_add(to_add, we)) {
            return WRDE_NOSPACE;
        }

        if (s[i] == '\0') {
            break;
        }

        i += to_advance;
        if (s[i] == '\0') {
            break;
        }

        prev = i--; // Since loop does i++
    }

    return 0;
}

int wordexp(const char *s, wordexp_t *p, int flags) {
    assert(!(flags & WRDE_REUSE));
    assert(!(flags & WRDE_APPEND));
    assert(!(flags & WRDE_DOOFFS));

    memset(p, 0, sizeof(wordexp_t));

    char *split_on = getenv("IFS");
    if (!split_on) {
        split_on = " \t\n";
    }

    char *str = strdup(s);
    int ret = we_split(str, split_on, p);
    free(str);

    return ret;
}

void wordfree(wordexp_t *p) {
    for (size_t i = p->we_offs; i < p->we_wordc; i++) {
        if (p->we_wordv[i] != NULL) {
            free(p->we_wordv[i]);
        }
    }

    free(p->we_wordv);
}