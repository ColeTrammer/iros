#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wordexp.h>

#define WE_BUF_INCREMENT 10

static bool we_add(char *s, wordexp_t *we) {
    if (we->we_wordc == 0) {
        we->we_wordv = calloc(WE_BUF_INCREMENT, sizeof(char *));
    } else if (we->we_wordc % WE_BUF_INCREMENT == 0) {
        we->we_wordv = realloc(we->we_wordv, (we->we_wordc + WE_BUF_INCREMENT) * sizeof(char *));
    }

    // Memory allocation error
    if (we->we_wordv == NULL) {
        return false;
    }

    we->we_wordv[we->we_wordc++] = s;
    we->we_wordv[we->we_wordc] = NULL;
    return true;
}

// Overwrite entry at pos, move over everything else
static bool we_insert(char **arr, size_t arr_size, size_t pos, wordexp_t *we) {
    assert(arr_size != 0);
    size_t new_size = we->we_wordc - 1 + arr_size;
    if (we->we_wordc / WE_BUF_INCREMENT != new_size / WE_BUF_INCREMENT) {
        size_t new_max_length = WE_BUF_INCREMENT * ((new_size + WE_BUF_INCREMENT - 1) / WE_BUF_INCREMENT);
        we->we_wordv = realloc(we->we_wordv, new_max_length * sizeof(char *));
    }

    if (we->we_wordv == NULL) {
        return false;
    }

    for (size_t i = we->we_wordc - 1; i > pos; i--) {
        we->we_wordv[new_size - (we->we_wordc - i)] = we->we_wordv[i];
    }

    for (size_t i = pos; i < pos + arr_size; i++) {
        we->we_wordv[i] = strdup(arr[i - pos]);
        if (we->we_wordv[i] == NULL) {
            return false;
        }
    }

    we->we_wordc = new_size;
    we->we_wordv[we->we_wordc] = NULL;
    return true;
}

#define WE_STR_BUF_INCREMENT 0x200

static bool we_append(char **s, const char *r, size_t len, size_t *max) {
    size_t new_len = strlen(*s) + len + 1;
    if (new_len > *max) {
        *max += WE_STR_BUF_INCREMENT;
        *s = realloc(*s, *max);
        if (!*s) {
            return false;
        }
    }

    strncat(*s, r, len);
    return true;
}

static int we_expand(const char *s, int flags, char **expanded, word_special_t *special) {
    size_t len = WE_STR_BUF_INCREMENT;
    *expanded = calloc(len, sizeof(char));

    bool prev_was_backslash = false;
    bool in_s_quotes = false;
    bool in_d_quotes = false;
    for (size_t i = 0; s[i] != '\0'; i++) {
        switch (s[i]) {
            case '\\': {
                if (!prev_was_backslash) {
                    prev_was_backslash = true;
                    continue;
                }
                break;
            }
            case '\'': {
                in_s_quotes = (prev_was_backslash || in_d_quotes) ? in_s_quotes : !in_s_quotes;
                break;
            }
            case '"': {
                in_d_quotes = (prev_was_backslash || in_s_quotes) ? in_d_quotes : !in_d_quotes;
                break;
            }
            case '$': {
                if (!(flags & WRDE_SPECIAL) || special == NULL) {
                    goto normal_var;
                }

                char *to_add;
                switch (s[i + 1]) {
                    case '@':
                        to_add = special->vals[WRDE_SPECIAL_AT];
                        break;
                    case '*':
                        to_add = special->vals[WRDE_SPECIAL_STAR];
                        break;
                    case '#':
                        to_add = special->vals[WRDE_SPECIAL_POUND];
                        break;
                    case '?':
                        to_add = special->vals[WRDE_SPECIAL_QUEST];
                        break;
                    case '-':
                        to_add = special->vals[WRDE_SPECIAL_MINUS];
                        break;
                    case '$':
                        to_add = special->vals[WRDE_SPECIAL_DOLLAR];
                        break;
                    case '!':
                        to_add = special->vals[WRDE_SPECIAL_EXCLAM];
                        break;
                    case '0':
                        to_add = special->vals[WRDE_SPECIAL_ZERO];
                        break;
                    default:
                        goto normal_var;
                }

                if (!we_append(expanded, to_add, strlen(to_add), &len)) {
                    return WRDE_NOSPACE;
                }
                i += 2;
                i--;
                prev_was_backslash = false;
                continue;
            normal_var : {
                // Maybe other characters are valid but this is the standard form
                int to_read = strspn(s + i + 1, "_ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");

                // NOTE: Still does expansion if in_d_quotes
                if (prev_was_backslash || in_s_quotes || to_read <= 0) {
                    break;
                }

                i++;
                char save = s[i + to_read];
                ((char *) s)[i + to_read] = '\0';

                char *var = getenv(s + i);
                if (var != NULL) {
                    we_append(expanded, var, strlen(var), &len);
                } else if (flags & WRDE_UNDEF) {
                    ((char *) s)[i + to_read] = save;
                    free(*expanded);
                    return WRDE_BADVAL;
                }

                ((char *) s)[i + to_read] = save;
                i += to_read;
                i--; // Since loop does i++

                prev_was_backslash = false;
                continue;
            }
            }
            case '`': {
                if (prev_was_backslash || in_s_quotes) {
                    break;
                }

                // Doesn't handle escaped '`' but whatever
                char *end = strchr(s + i + 1, '`');
                if (end == NULL) {
                    free(*expanded);
                    return WRDE_SYNTAX;
                }

                char save = *end;
                *end = '\0';

                int save_stderr = 0;

                // Handle redirecting error
                if (!(flags & WRDE_SHOWERR)) {
                    save_stderr = dup(STDERR_FILENO);
                    int fd = open("/dev/null", O_RDWR);
                    dup2(fd, STDERR_FILENO);
                    close(fd);
                }
                FILE *_pipe = popen(s + i + 1, "r");
                if (_pipe == NULL) {
                    goto bquote_end;
                }

                char *line = NULL;
                size_t line_len = 0;
                while (getline(&line, &line_len, _pipe) != -1) {
                    if (!we_append(expanded, line, strlen(line), &len)) {
                        assert(false);
                        return WRDE_NOSPACE;
                    }
                }

                free(line);
                pclose(_pipe);

            bquote_end:
                if (!(flags & WRDE_SHOWERR)) {
                    dup2(save_stderr, STDERR_FILENO);
                    close(save_stderr);
                }

                *end = save;
                i = end - s;
                prev_was_backslash = false;
                continue;
            }
            case '~': {
                char *home = getenv("HOME");
                if (prev_was_backslash || in_s_quotes || in_d_quotes || home == NULL) {
                    break;
                }

                if (!we_append(expanded, home, strlen(home), &len)) {
                    return WRDE_NOSPACE;
                }

                prev_was_backslash = false;
                continue;
            }
            default: {
                break;
            }
        }

        if (!we_append(expanded, s + i, 1, &len)) {
            return WRDE_NOSPACE;
        }

        prev_was_backslash = false;
    }

    return 0;
}

static int we_split(char *s, char *split_on, wordexp_t *we) {
    bool prev_was_blackslash = false;
    bool in_s_quotes = false;
    bool in_d_quotes = false;

    int prev = strspn(s, split_on);
    for (size_t i = prev;; i++) {
        switch (s[i]) {
            case '\\':
                if (prev_was_blackslash) {
                    prev_was_blackslash = false;
                    break;
                }
                prev_was_blackslash = true;
                continue;
            case '\'':
                in_s_quotes = (prev_was_blackslash || in_d_quotes) ? in_s_quotes : !in_s_quotes;
                break;
            case '"':
                in_d_quotes = (prev_was_blackslash || in_s_quotes) ? in_d_quotes : !in_d_quotes;
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

static int we_unescape(wordexp_t *p) {
    for (size_t i = 0; i < p->we_wordc; i++) {
        char *unescaped_string = calloc(strlen(p->we_wordv[i]) + 1, sizeof(char));
        if (unescaped_string == NULL) {
            return WRDE_NOSPACE;
        }

        bool in_s_quotes = false;
        bool in_d_quotes = false;
        for (size_t j = 0, k = 0; p->we_wordv[i][j] != '\0'; j++, k++) {
        again:
            switch (p->we_wordv[i][j]) {
                case '\\':
                    j++;
                    break;
                case '\'':
                    if (!in_d_quotes) {
                        j++;
                        in_s_quotes = in_d_quotes ? in_s_quotes : !in_s_quotes;
                        goto again;
                    }
                    break;
                case '"':
                    if (!in_s_quotes) {
                        j++;
                        in_d_quotes = in_s_quotes ? in_d_quotes : !in_d_quotes;
                        goto again;
                    }
                    break;
                default:
                    break;
            }

            unescaped_string[k] = p->we_wordv[i][j];
        }

        free(p->we_wordv[i]);
        p->we_wordv[i] = unescaped_string;
    }

    return 0;
}

static int we_glob(wordexp_t *we) {
    for (size_t i = 0; i < we->we_wordc; i++) {
        char *token = we->we_wordv[i];

        bool prev_was_bachslash = false;
        bool in_s_qutoes = false;
        bool in_d_quotes = false;
        for (size_t j = 0; token[j] != '\0'; j++) {
            switch (token[j]) {
                case '\\': {
                    if (!prev_was_bachslash) {
                        prev_was_bachslash = true;
                        continue;
                    }
                    break;
                }
                case '\'': {
                    in_s_qutoes = (prev_was_bachslash || in_d_quotes) ? in_s_qutoes : !in_s_qutoes;
                    break;
                }
                case '"': {
                    in_d_quotes = (prev_was_bachslash || in_s_qutoes) ? in_d_quotes : !in_d_quotes;
                    break;
                }
                case '*':
                case '?':
                case '[': {
                    if (prev_was_bachslash || in_d_quotes || in_s_qutoes) {
                        break;
                    }
                    // For now, assume entire token is the pattern (not necessarily the case)
                    glob_t gl;
                    int err = 0;
                    if ((err = glob(token, 0, NULL, &gl))) {
                        if (err == GLOB_NOSPACE) {
                            return WRDE_NOSPACE;
                        }
                        break;
                    }

                    free(token);
                    if (!we_insert(gl.gl_pathv, gl.gl_pathc, i, we)) {
                        globfree(&gl);
                        return WRDE_NOSPACE;
                    }
                    globfree(&gl);
                    i += gl.gl_pathc;
                    i--; // since loop does i++
                    break;
                }
                default: {
                    break;
                }
            }

            prev_was_bachslash = false;
        }
    }

    return 0;
}

int wordexp(const char *s, wordexp_t *p, int flags) {
    assert(!(flags & WRDE_REUSE));
    assert(!(flags & WRDE_APPEND));
    assert(!(flags & WRDE_DOOFFS));

    p->we_offs = p->we_wordc = 0;
    p->we_wordv = NULL;

    char *str = NULL;
    int ret = we_expand(s, flags, &str, p->we_special_vars);
    if (ret != 0) {
        return ret;
    }

    char *split_on = getenv("IFS");
    if (!split_on) {
        split_on = " \t\n";
    }

    assert(str);
    ret = we_split(str, split_on, p);
    free(str);

    if (ret != 0) {
        return ret;
    }

    assert(p->we_wordv);
    ret = we_glob(p);

    if (ret != 0) {
        wordfree(p);
        return ret;
    }

    return we_unescape(p);
}

void wordfree(wordexp_t *p) {
    for (size_t i = p->we_offs; i < p->we_wordc; i++) {
        if (p->we_wordv[i] != NULL) {
            free(p->we_wordv[i]);
        }
    }

    free(p->we_wordv);
}