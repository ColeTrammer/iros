#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <glob.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef USERLAND_NATIVE
#include <wordexp.h>
#else
#include "../include/wordexp.h"
#endif /* USERLAND_NATIVE */

#define WE_BUF_INCREMENT 10

#define WE_EXPAND_MAX_DEPTH 1024

enum param_expansion_type { EXPAND_BRACE, EXPAND_DOUBLE_PARAN, EXPAND_SINGLE_PAREN };

size_t we_find_end_of_word_expansion(const char *input_stream, size_t start, size_t input_length) {
    bool in_b_quotes = false;
    bool in_s_quotes = false;
    bool in_d_quotes = false;
    bool prev_was_backslash = false;
    bool prev_was_dollar = false;

    // Max depth of recursion
    enum param_expansion_type type_stack[1024];
    size_t type_stack_index = 0;

    do {
        char current = input_stream[start];
        switch (current) {
            case '\\':
                if (!in_s_quotes) {
                    prev_was_backslash = !prev_was_backslash;
                    prev_was_dollar = false;
                    continue;
                }
                break;
            case '$':
                if (!prev_was_backslash && !in_d_quotes && !in_s_quotes && !in_b_quotes && !prev_was_dollar) {
                    prev_was_dollar = true;
                    continue;
                }
                break;
            case '\'':
                in_s_quotes = !in_d_quotes && !in_b_quotes ? !in_s_quotes : in_s_quotes;
                break;
            case '"':
                in_d_quotes = !prev_was_backslash && !in_s_quotes && !in_b_quotes ? !in_d_quotes : in_d_quotes;
                break;
            case '`':
                in_b_quotes = !prev_was_backslash && !in_d_quotes && !in_s_quotes ? !in_b_quotes : in_b_quotes;
                break;
            case '{':
                if (prev_was_dollar && !prev_was_backslash && !in_d_quotes && !in_s_quotes && !in_b_quotes) {
                    type_stack[type_stack_index++] = EXPAND_BRACE;
                }
                break;
            case '(':
                if (!prev_was_backslash && !in_d_quotes && !in_s_quotes && !in_b_quotes) {
                    if (start + 1 < input_length && input_stream[start + 1] == '(') {
                        start++;
                        type_stack[type_stack_index++] = EXPAND_DOUBLE_PARAN;
                    } else {
                        type_stack[type_stack_index++] = EXPAND_SINGLE_PAREN;
                    }
                }
                break;
            case '}':
                if (!prev_was_backslash && !in_d_quotes && !in_s_quotes && !in_b_quotes) {
                    if (type_stack_index == 0 || type_stack[--type_stack_index] != EXPAND_BRACE) {
                        return 0;
                    }
                }
                break;
            case ')':
                if (!prev_was_backslash && !in_d_quotes && !in_s_quotes && !in_b_quotes) {
                    if (start + 1 < input_length && input_stream[start + 1] == ')') {
                        if (type_stack_index == 0 || type_stack[type_stack_index] != EXPAND_DOUBLE_PARAN) {
                            goto try_single_rparen;
                        }
                        type_stack_index--;
                        start++;
                    } else {
                    try_single_rparen:
                        if (type_stack_index == 0 || type_stack[--type_stack_index] != EXPAND_SINGLE_PAREN) {
                            return 0;
                        }
                    }
                }
                break;
            default:
                break;
        }

        prev_was_backslash = false;
        prev_was_dollar = false;
    } while (++start < input_length && (in_d_quotes || in_b_quotes || in_s_quotes || type_stack_index != 0 || prev_was_dollar));

    return !in_d_quotes && !in_b_quotes && !in_s_quotes && type_stack_index == 0 ? start - 1 : 0;
}

int we_add(char *s, wordexp_t *we) {
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
int we_insert(char **arr, size_t arr_size, size_t pos, wordexp_t *we) {
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
    if (len == 0) {
        return true;
    }

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

static int we_command_subst(const char *to_expand, int flags, char **expanded, size_t *len) {
    if (flags & WRDE_NOCMD) {
        return WRDE_CMDSUB;
    }

    int save_stderr = 0;
    int ret = 0;

    // Handle redirecting error
    if (!(flags & WRDE_SHOWERR)) {
        save_stderr = dup(STDERR_FILENO);
        int fd = open("/dev/null", O_RDWR);
        dup2(fd, STDERR_FILENO);
        close(fd);
    }
    FILE *_pipe = popen(to_expand, "r");
    if (_pipe == NULL) {
        goto cleanup_command_subst;
    }

    char *line = NULL;
    size_t line_len = 0;
    while (getline(&line, &line_len, _pipe) != -1) {
        if (!we_append(expanded, line, strlen(line), len)) {
            ret = WRDE_NOSPACE;
            goto cleanup_command_subst_and_pipes;
        }
    }

cleanup_command_subst_and_pipes:
    free(line);
    pclose(_pipe);

cleanup_command_subst:
    if (!(flags & WRDE_SHOWERR)) {
        dup2(save_stderr, STDERR_FILENO);
        close(save_stderr);
    }

    return ret;
}

struct param_expansion_result {
    char *name;
    char *result;
    bool should_free_result;
};

void we_param_expand_free(struct param_expansion_result *result) {
    free(result->name);

    if (result->should_free_result) {
        free(result->result);
    }
}

static int we_get_value_for_name(struct param_expansion_result *result, int flags, word_special_t *special) {
    if ((flags & WRDE_SPECIAL) && special) {
        switch (result->name[0]) {
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9': {
                int index = atoi(result->name);
                if (index == 0) {
                    return WRDE_SYNTAX;
                }

                if (index - 1 >= (int) special->position_args_size) {
                    result->result = NULL;
                    return 0;
                }

                result->result = special->position_args[index - 1];
                return 0;
            }
            case '$':
                result->result = special->vals[WRDE_SPECIAL_DOLLAR];
                return 0;
            case '?':
                result->result = special->vals[WRDE_SPECIAL_QUEST];
                return 0;
            case '-':
                result->result = special->vals[WRDE_SPECIAL_MINUS];
                return 0;
            case '!':
                result->result = special->vals[WRDE_SPECIAL_EXCLAM];
                return 0;
            case '0':
                result->result = special->vals[WRDE_SPECIAL_ZERO];
                return 0;
            case '#': {
                char buf[20];
                snprintf(buf, 19, "%lu", special->position_args_size);
                result->result = strdup(buf);
                if (!result->result) {
                    return WRDE_NOSPACE;
                }

                result->should_free_result = true;
                return 0;
            }
            // FIXME: $@ needs to be special cased even more when surrounded by ""
            case '@':
            case '*': {
                if (special->position_args_size == 0) {
                    result->result = "";
                    return 0;
                }

                char split_char[2];
                strcpy(split_char, " ");
                if (result->name[0] == '*') {
                    char *ifs = getenv("IFS");
                    if (ifs) {
                        split_char[0] = ifs[0];
                    }
                }

                size_t len = 0;
                for (size_t i = 0; i < special->position_args_size; i++) {
                    len += strlen(special->position_args[i]);
                    len += i != special->position_args_size - 1;
                }

                result->result = malloc(len + 1);
                if (!result->result) {
                    return WRDE_NOSPACE;
                }

                result->should_free_result = true;
                result->result[0] = '\0';
                for (size_t i = 0; i < special->position_args_size; i++) {
                    strcat(result->result, special->position_args[i]);
                    if (i != special->position_args_size - 1) {
                        strcat(result->result, split_char);
                    }
                }

                return 0;
            }
            default:
                break;
        }
    }

    result->result = getenv(result->name);
    return 0;
}

enum param_expand_type {
    DEFAULT,
    ASSIGN_DEFAULT,
    ERROR_IF_UNSET,
    USE_ALTERNATE_VALUE,
    REMOVE_SMALLEST_SUFFIX,
    REMOVE_LARGEST_SUFFIX,
    REMOVE_SMALLEST_PREFIX,
    REMOVE_LARGEST_PREFIX
};

// Takes in expression of form ${name[op][word]}
static int we_param_expand(const char *s, size_t length, int flags, word_special_t *special, struct param_expansion_result *result) {
    result->name = NULL;
    result->result = NULL;
    result->should_free_result = false;

    int err = 0;
    if (length <= 3 || s[0] != '$' || s[1] != '{' || s[length - 1] != '}') {
        return WRDE_SYNTAX;
    }

    s += 2;
    length -= 3;
    size_t name_i = 0;

    bool only_finding_length = false;
    switch (s[0]) {
        case '#':
            if (s[1] == '}' || s[1] == ':' || s[1] == '=' || s[1] == '-' || s[1] == '?' || s[1] == '+') {
                name_i++;
                goto found_name;
            }
            s++;
            length--;
            only_finding_length = true;
            break;
        case '@':
        case '*':
        case '?':
        case '-':
        case '$':
        case '!':
            name_i++;
            goto found_name;
        default:
            break;
    }

    for (; name_i < length; name_i++) {
        if (!isalpha(s[name_i]) && s[name_i] != '_' && !isdigit(s[name_i])) {
            break;
        }
    }

found_name:
    result->name = malloc(name_i + 1);
    if (result->name == NULL) {
        return WRDE_NOSPACE;
    }

    memcpy(result->name, s, name_i);
    result->name[name_i] = '\0';

    // This means name is entirely included by {}
    if (name_i == length) {
        int ret = we_get_value_for_name(result, flags, special);
        if (ret != 0) {
            err = ret;
            goto we_param_expand_fail;
        }

        if (!result->result && (flags & WRDE_UNDEF)) {
            err = WRDE_BADVAL;
            goto we_param_expand_fail;
        }

        if (only_finding_length) {
            size_t len = 0;
            if (result->result) {
                len = strlen(result->result);
            }

            if (result->should_free_result) {
                free(result->result);
            }

            char buf[50];
            snprintf(buf, 49, "%lu", len);
            result->result = strdup(buf);
            if (!result->result) {
                err = WRDE_NOSPACE;
                goto we_param_expand_fail;
            }

            result->should_free_result = true;
            return 0;
        }

        return 0;
    } else if (only_finding_length) {
        err = WRDE_SYNTAX;
        goto we_param_expand_fail;
    }

    bool consider_empty_to_be_unset = s[name_i] == ':';
    if (consider_empty_to_be_unset) {
        name_i++;
    }

    enum param_expand_type type;
    switch (s[name_i]) {
        case '-':
            type = DEFAULT;
            break;
        case '=':
            type = ASSIGN_DEFAULT;
            break;
        case '?':
            type = ERROR_IF_UNSET;
            break;
        case '+':
            type = USE_ALTERNATE_VALUE;
            break;
        case '%':
            if (s[name_i + 1] != '%') {
                type = REMOVE_SMALLEST_SUFFIX;
            } else {
                name_i++;
                type = REMOVE_LARGEST_SUFFIX;
            }
            break;
        case '#':
            if (s[name_i + 1] != '#') {
                type = REMOVE_SMALLEST_PREFIX;
            } else {
                name_i++;
                type = REMOVE_LARGEST_PREFIX;
            }
            break;
        default:
            err = WRDE_SYNTAX;
            goto we_param_expand_fail;
    }

    name_i++;

    int ret = we_get_value_for_name(result, flags, special);
    if (ret != 0) {
        err = ret;
        goto we_param_expand_fail;
    }

    bool is_set = result->result && !(consider_empty_to_be_unset && result->result[0] == '\0');

    if ((type == USE_ALTERNATE_VALUE && is_set) || !is_set || type >= REMOVE_SMALLEST_SUFFIX) {
        bool is_word_to_expand = name_i != length;
        bool need_to_free_expanded_word = false;
        char *expanded_word = "";
        if (is_word_to_expand) {
            need_to_free_expanded_word = true;
            char save = s[length];
            ((char *) s)[length] = '\0';
            int ret = we_expand(s + name_i, flags, &expanded_word, special);
            ((char *) s)[length] = save;

            if (ret == 0) {
                ret = we_unescape(&expanded_word);
            }

            if (ret != 0) {
                free(expanded_word);
                err = ret;
                goto we_param_expand_fail;
            }
        }

        if (type >= REMOVE_SMALLEST_SUFFIX) {
            if (!result->result && (flags & WRDE_UNDEF)) {
                err = WRDE_BADVAL;
                goto we_param_expand_fail;
            }

            if (!is_word_to_expand || !result->result || result->result[0] == '\0') {
                return 0;
            }

            size_t result_len = strlen(result->result);
            switch (type) {
                case REMOVE_SMALLEST_SUFFIX: {
                    for (ssize_t i = result_len; i >= 0; i--) {
                        int ret = fnmatch(expanded_word, result->result + i, 0);
                        if (ret == FNM_NOMATCH) {
                            continue;
                        } else if (ret != 0) {
                            err = WRDE_SYNTAX;
                            goto we_param_expand_fail;
                        }

                        if (result->should_free_result) {
                            free(result->result);
                            result->should_free_result = false;
                        }

                        result->result = strdup(result->result);
                        if (!result->result) {
                            err = WRDE_NOSPACE;
                            goto we_param_expand_fail;
                        }

                        result->should_free_result = true;
                        result->result[i] = '\0';
                        return 0;
                    }

                    goto add_pattern;
                }
                case REMOVE_LARGEST_SUFFIX: {
                    for (ssize_t i = 0; i <= result_len; i++) {
                        int ret = fnmatch(expanded_word, result->result + i, 0);
                        if (ret == FNM_NOMATCH) {
                            continue;
                        } else if (ret != 0) {
                            err = WRDE_SYNTAX;
                            goto we_param_expand_fail;
                        }

                        if (result->should_free_result) {
                            free(result->result);
                            result->should_free_result = false;
                        }

                        result->result = strdup(result->result);
                        if (!result->result) {
                            err = WRDE_NOSPACE;
                            goto we_param_expand_fail;
                        }

                        result->should_free_result = true;
                        result->result[i] = '\0';
                        return 0;
                    }

                    goto add_pattern;
                }
                case REMOVE_SMALLEST_PREFIX: {
                    char *attempt_word = strdup(result->result);
                    if (!attempt_word) {
                        err = WRDE_NOSPACE;
                        goto we_param_expand_fail;
                    }

                    for (ssize_t i = 0; i < result_len; i++) {
                        char save = attempt_word[i];
                        attempt_word[i] = '\0';

                        int ret = fnmatch(expanded_word, attempt_word, 0);
                        attempt_word[i] = save;
                        if (ret == FNM_NOMATCH) {
                            continue;
                        } else if (ret != 0) {
                            free(attempt_word);
                            err = WRDE_SYNTAX;
                            goto we_param_expand_fail;
                        }

                        if (result->should_free_result) {
                            free(result->result);
                            result->should_free_result = false;
                        }

                        result->result = strdup(result->result + i);
                        if (!result->result) {
                            err = WRDE_NOSPACE;
                            goto we_param_expand_fail;
                        }

                        result->should_free_result = true;
                        return 0;
                    }

                    free(attempt_word);
                    goto add_pattern;
                }
                case REMOVE_LARGEST_PREFIX: {
                    char *attempt_word = strdup(result->result);
                    if (!attempt_word) {
                        err = WRDE_NOSPACE;
                        goto we_param_expand_fail;
                    }

                    for (ssize_t i = result_len; i >= 0; i--) {
                        char save = attempt_word[i];
                        attempt_word[i] = '\0';

                        int ret = fnmatch(expanded_word, attempt_word, 0);
                        attempt_word[i] = save;
                        if (ret == FNM_NOMATCH) {
                            continue;
                        } else if (ret != 0) {
                            free(attempt_word);
                            err = WRDE_SYNTAX;
                            goto we_param_expand_fail;
                        }

                        if (result->should_free_result) {
                            free(result->result);
                            result->should_free_result = false;
                        }

                        result->result = strdup(result->result + i);
                        if (!result->result) {
                            err = WRDE_NOSPACE;
                            goto we_param_expand_fail;
                        }

                        result->should_free_result = true;
                        return 0;
                    }

                    free(attempt_word);
                    goto add_pattern;
                }
            }

            assert(false);
        }

    add_pattern:
        if (result->should_free_result) {
            free(result->result);
            result->should_free_result = false;
        }

        result->result = expanded_word;
        result->should_free_result = need_to_free_expanded_word;
        if (type == ERROR_IF_UNSET) {
            if (is_word_to_expand) {
                fputs(result->result, stderr);
            } else {
                fprintf(stderr, "%s: parameter not set\n", result->name);
            }
            err = WRDE_CMDSUB;
            goto we_param_expand_fail;
        }

        if (type == ASSIGN_DEFAULT) {
            setenv(result->name, result->result, 1);
        }

        return 0;
    }

    return 0;

we_param_expand_fail:
    we_param_expand_free(result);
    return err;
}

int we_expand(const char *s, int flags, char **expanded, word_special_t *special) {
    size_t len = WE_STR_BUF_INCREMENT;
    size_t input_len = strlen(s);
    *expanded = calloc(len, sizeof(char));

    bool prev_was_backslash = false;
    bool in_s_quotes = false;
    bool in_d_quotes = false;
    for (size_t i = 0; s[i] != '\0'; i++) {
        switch (s[i]) {
            case '\\': {
                if (!prev_was_backslash && !in_s_quotes) {
                    prev_was_backslash = true;

                    if (!we_append(expanded, s + i, 1, &len)) {
                        return WRDE_NOSPACE;
                    }
                    continue;
                }
                break;
            }
            case '\'': {
                in_s_quotes = in_d_quotes ? in_s_quotes : !in_s_quotes;
                break;
            }
            case '"': {
                in_d_quotes = (prev_was_backslash || in_s_quotes) ? in_d_quotes : !in_d_quotes;
                break;
            }
            case '$': {
                // Command sub
                if (s[i + 1] == '(') {
                    const char *to_expand = s + i + 2;
                    i = we_find_end_of_word_expansion(s, i, input_len);
                    if (i == 0) {
                        free(*expanded);
                        return WRDE_SYNTAX;
                    }

                    char save = s[i];
                    ((char *) s)[i] = '\0';

                    int ret = we_command_subst(to_expand, flags, expanded, &len);

                    ((char *) s)[i] = save;

                    if (ret != 0) {
                        free(*expanded);
                        return ret;
                    }

                    prev_was_backslash = false;
                    continue;
                }

                if (s[i + 1] == '{') {
                    size_t end = we_find_end_of_word_expansion(s, i, input_len);
                    if (end == 0) {
                        free(*expanded);
                        return WRDE_SYNTAX;
                    }

                    struct param_expansion_result result;
                    int ret = we_param_expand(s + i, end - i + 1, flags, special, &result);
                    if (ret != 0) {
                        free(*expanded);
                        return ret;
                    }

                    if (!result.result) {
                        result.result = "";
                    }

                    if (!we_append(expanded, result.result, strlen(result.result), &len)) {
                        we_param_expand_free(&result);
                        return WRDE_NOSPACE;
                    }

                    we_param_expand_free(&result);
                    i = end;
                    prev_was_backslash = false;
                    continue;
                }

                if (!(flags & WRDE_SPECIAL) || special == NULL) {
                    goto normal_var;
                }

                char *to_add;
                switch (s[i + 1]) {
                    case '@':
                    case '*':
                        for (size_t i = 0; i < special->position_args_size; i++) {
                            if (!we_append(expanded, special->position_args[i], strlen(special->position_args[i]), &len)) {
                                return WRDE_NOSPACE;
                            }
                            if (i != special->position_args_size - 1) {
                                if (!we_append(expanded, " ", 1, &len)) {
                                    return WRDE_NOSPACE;
                                }
                            }
                        }
                        goto finish_special_var;
                    case '#': {
                        char buf[50];
                        snprintf(buf, 49, "%lu", special->position_args_size);
                        if (!we_append(expanded, buf, strlen(buf), &len)) {
                            return WRDE_NOSPACE;
                        }
                        goto finish_special_var;
                    }
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
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                        if ((size_t)(s[i + 1] - '1') >= special->position_args_size) {
                            to_add = "";
                        } else {
                            to_add = special->position_args[s[i + 1] - '1'];
                        }
                        break;
                    default:
                        goto normal_var;
                }

                if (!we_append(expanded, to_add, strlen(to_add), &len)) {
                    return WRDE_NOSPACE;
                }

            finish_special_var:
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
                const char *end = NULL;
                char *to_expand = NULL;
                size_t to_expand_index = 0;
                size_t to_expand_max = 0;
                {
                    bool prev_was_backslash = false;
                    size_t index = i + 1;
                    for (;; index++) {
                        switch (s[index]) {
                            case '\0':
                                free(to_expand);
                                free(*expanded);
                                return WRDE_SYNTAX;
                            case '\\':
                                prev_was_backslash = !prev_was_backslash;
                                continue;
                            case '`':
                                if (!prev_was_backslash) {
                                    goto found_end_bquote;
                                }
                                prev_was_backslash = false;
                                // Fall through
                            default: {
                                if (to_expand_index + 2 >= to_expand_max) {
                                    to_expand_max += 20;
                                    to_expand = realloc(to_expand, to_expand_max);
                                }

                                if (prev_was_backslash) {
                                    to_expand[to_expand_index++] = '\\';
                                    prev_was_backslash = false;
                                }

                                to_expand[to_expand_index++] = s[index];
                            }
                        }
                    }

                found_end_bquote:
                    end = s + index;
                    if (to_expand_index == 0) {
                        goto bquote_end;
                    }

                    to_expand[to_expand_index] = '\0';
                }

                if (end == NULL) {
                    free(*expanded);
                    return WRDE_SYNTAX;
                }

                int ret = we_command_subst(to_expand, flags, expanded, &len);
                if (ret != 0) {
                    free(*expanded);
                    free(to_expand);
                    return ret;
                }

            bquote_end:
                free(to_expand);
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
                if (!in_s_quotes) {
                    if (prev_was_blackslash) {
                        prev_was_blackslash = false;
                        break;
                    }
                    prev_was_blackslash = true;
                    continue;
                }
                break;
            case '\'':
                in_s_quotes = in_d_quotes ? in_s_quotes : !in_s_quotes;
                break;
            case '"':
                in_d_quotes = (prev_was_blackslash || in_s_quotes) ? in_d_quotes : !in_d_quotes;
                break;
            default:
                break;
        }

        if ((prev_was_blackslash && s[i] != '\0') || in_s_quotes || in_d_quotes) {
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

int we_unescape(char **s) {
    char *unescaped_string = NULL;
    size_t unescaped_string_max = 0;

    bool in_s_quotes = false;
    bool in_d_quotes = false;
    size_t k = 0;
    for (size_t j = 0; (*s)[j] != '\0'; j++, k++) {
    again:
        switch ((*s)[j]) {
            case '\\':
                if (!in_s_quotes) {
                    j++;
                    if ((*s)[j] == '\0') {
                        j--;
                        break;
                    }
                    break;
                }
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

        if (k + 1 >= unescaped_string_max) {
            unescaped_string_max += 20;
            unescaped_string = realloc(unescaped_string, unescaped_string_max);
            if (unescaped_string == NULL) {
                return WRDE_NOSPACE;
            }
        }
        unescaped_string[k] = (*s)[j];
    }

    free(*s);

    if (unescaped_string == NULL) {
        unescaped_string = strdup("");
        if (unescaped_string == NULL) {
            return WRDE_NOSPACE;
        }
    }
    unescaped_string[k] = '\0';
    (*s) = unescaped_string;
    return 0;
}

static int we_unescape_all(wordexp_t *p) {
    for (size_t i = 0; i < p->we_wordc; i++) {
        int ret = we_unescape(&p->we_wordv[i]);
        if (ret != 0) {
            return ret;
        }
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
    assert(!(flags & WRDE_DOOFFS));

    if (!(flags & WRDE_APPEND)) {
        p->we_offs = p->we_wordc = 0;
        p->we_wordv = NULL;
    }

#ifdef WORDEXP_DEBUG
    fprintf(stderr, "expanding: |%s|\n", s);
#endif /* WORDEXP_DEBUG */

    char *str = NULL;
    int ret = we_expand(s, flags, &str, p->we_special_vars);
    if (ret != 0) {
        return ret;
    }

#ifdef WORDEXP_DEBUG
    fprintf(stderr, "expand result: |%s|\n", str);
#endif /* WORDEXP_DEBUG */

    // Nothing more to do if the result of we_expand is the empty string
    if (str[0] == '\0') {
        return 0;
    }

    char *split_on = NULL;
    if (flags & WRDE_NOFS) {
        split_on = "";
    } else {
        split_on = getenv("IFS");
        if (!split_on) {

            split_on = " \t\n";
        }
    }

    assert(str);
    ret = we_split(str, split_on, p);
    free(str);

    if (ret != 0) {
        return ret;
    }

#ifdef WORDEXP_DEBUG
    for (size_t i = 0; i < p->we_wordc; i++) {
        fprintf(stderr, "split result: %lu:|%s|\n", i, p->we_wordv[i]);
    }
#endif /* WORDEXP_DEBUG */

    assert(p->we_wordv);

    if (!(flags & WRDE_NOGLOB)) {
        ret = we_glob(p);

        if (ret != 0) {
            wordfree(p);
            return ret;
        }

#ifdef WORDEXP_DEBUG
        for (size_t i = 0; i < p->we_wordc; i++) {
            fprintf(stderr, "glob result: %lu:|%s|\n", i, p->we_wordv[i]);
        }
#endif /* WORDEXP_DEBUG */
    }

    ret = we_unescape_all(p);

#ifdef WORDEXP_DEBUG
    for (size_t i = 0; i < p->we_wordc; i++) {
        fprintf(stderr, "unescape result: %lu:|%s|\n", i, p->we_wordv[i]);
    }
#endif /* WORDEXP_DEBUG */

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