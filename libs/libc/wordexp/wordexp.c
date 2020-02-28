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
#include <sys/wait.h>
#include <unistd.h>

#ifndef USERLAND_NATIVE
#include <wordexp.h>
#else
#include "../include/wordexp.h"
#endif /* USERLAND_NATIVE */

#define WE_BUF_DEFAULT 16

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
                    if (start + 1 < input_length && input_stream[start + 1] == '(' && prev_was_dollar) {
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
                        if (type_stack_index == 0 || type_stack[type_stack_index - 1] != EXPAND_DOUBLE_PARAN) {
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
        we->we_wordv = calloc(WE_BUF_DEFAULT, sizeof(char *));
    } else if (we->we_wordc % 2 == 0) {
        we->we_wordv = realloc(we->we_wordv, (we->we_wordc * 2) * sizeof(char *));
    }

    // Memory allocation error
    if (we->we_wordv == NULL) {
        return false;
    }

    we->we_wordv[we->we_wordc++] = s;
    we->we_wordv[we->we_wordc] = NULL;
    return true;
}

// Overwriting position, inserting the rest, adds "" to the middle elements
// For use when expanding "$@"
// This overwritten string should be freed by the caller
bool we_insert_quoted(wordexp_t *we, size_t pos, char **insert_arr, size_t insert_arr_size) {
    assert(insert_arr_size >= 2);

    size_t new_size = we->we_wordc - 1 + insert_arr_size;
    if (we->we_wordc / 2 != new_size / 2) {
        size_t new_max_length = (new_size / 2 + 1) * 2;
        we->we_wordv = realloc(we->we_wordv, new_max_length * sizeof(char *));
    }

    if (we->we_wordv == NULL) {
        return false;
    }

    for (size_t i = we->we_wordc - 1; i > pos; i--) {
        we->we_wordv[new_size - (we->we_wordc - i)] = we->we_wordv[i];
    }

    for (size_t i = pos; i < pos + insert_arr_size; i++) {
        char *new_string = NULL;
        if (i == pos || i == pos + insert_arr_size - 1) {
            new_string = strdup(insert_arr[i - pos]);
        } else {
            new_string = malloc(strlen(insert_arr[i - pos]) + 3);
            if (new_string) {
                strcpy(new_string, "\"");
                strcat(new_string, insert_arr[i - pos]);
                strcat(new_string, "\"");
            }
        }

        if (!new_string) {
            return false;
        }

        we->we_wordv[i] = new_string;
    }

    we->we_wordc = new_size;
    we->we_wordv[we->we_wordc] = NULL;
    return true;
}

// Overwrite entry at pos, move over everything else
int we_insert(char **arr, size_t arr_size, size_t pos, wordexp_t *we) {
    assert(arr_size != 0);

    free(we->we_wordv[pos]);

    size_t new_size = we->we_wordc - 1 + arr_size;
    if (we->we_wordc / 2 != new_size / 2) {
        size_t new_max_length = (new_size / 2 + 1) * 2;
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

#define WE_STR_BUF_DEFAULT 32

static bool we_append(char **s, const char *r, size_t len, size_t *max) {
    if (len == 0) {
        return true;
    }

    size_t new_len = strlen(*s) + len + 1;
    if (new_len > *max) {
        if (*max == 0) {
            *max = WE_STR_BUF_DEFAULT;
        } else {
            *max *= 2;
        }
        *s = realloc(*s, *max);
        if (!*s) {
            return false;
        }
    }

    strncat(*s, r, len);
    return true;
}

static int we_command_subst(const char *to_expand, int flags, char **expanded, size_t *len, word_special_t *special) {
    if (flags & WRDE_NOCMD) {
        return WRDE_CMDSUB;
    }

    char *line = NULL;
    int save_stderr = 0;
    int ret = 0;

    // Handle redirecting error
    if (!(flags & WRDE_SHOWERR)) {
        save_stderr = dup(STDERR_FILENO);
        int fd = open("/dev/null", O_RDWR);
        dup2(fd, STDERR_FILENO);
        close(fd);
    }

    int fds[2];
    if (pipe(fds) < 0) {
        ret = WRDE_CMDSUB;
        goto cleanup_command_subst;
    }

    pid_t pid = fork();
    if (pid < 0) {
        ret = WRDE_CMDSUB;
        close(fds[0]);
        close(fds[1]);
        goto cleanup_command_subst;
    } else if (pid == 0) {
        dup2(fds[1], STDOUT_FILENO);
        close(fds[1]);
        close(fds[0]);
        if (special && special->do_command_subst) {
            _exit(special->do_command_subst((char *) to_expand));
        } else {
            _exit(execl("/bin/sh", "-c", to_expand, NULL));
        }
    }

    close(fds[1]);
    FILE *_pipe = fdopen(fds[0], "r");
    if (_pipe == NULL) {
        close(fds[0]);
        goto cleanup_command_subst;
    }

    size_t line_len = 0;
    while (getline(&line, &line_len, _pipe) != -1) {
        if (!we_append(expanded, line, strlen(line), len)) {
            ret = WRDE_NOSPACE;
            goto cleanup_command_subst_and_pipes;
        }
    }

    while (!waitpid(pid, NULL, 0))
        ;

cleanup_command_subst_and_pipes:
    free(line);
    fclose(_pipe);

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
static int we_param_expand(const char *s, size_t length, int flags, word_special_t *special, struct param_expansion_result *result,
                           bool in_d_quotes) {
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
            if (in_d_quotes && special && special->position_args_size > 1) {
                result->result = "$@";
                return 0;
            }
            // fall-through
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
                    for (ssize_t i = 0; i <= (ssize_t) result_len; i++) {
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

                    for (ssize_t i = 0; i < (ssize_t) result_len; i++) {
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
                default:
                    break;
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

int we_arithmetic_expand(const char *s, size_t length, int flags, word_special_t *special, long *value);

// Takes string of form [[!+-~]*]exp
int we_arithmetic_parse_terminal(const char *s, size_t max_length, char **name, int flags, word_special_t *special, long *value,
                                 const char **end_ptr) {
    *value = 0;
    *end_ptr = NULL;
    *name = NULL;

    if (max_length == 0) {
        return WRDE_SYNTAX;
    }

    switch (s[0]) {
        case '~': {
            int ret = we_arithmetic_parse_terminal(s + 1, max_length - 1, name, flags, special, value, end_ptr);
            if (ret != 0) {
                return ret;
            }

            if (*name != NULL) {
                free(*name);
                return WRDE_SYNTAX;
            }

            *value = ~*value;
            return 0;
        }
        case '!': {
            int ret = we_arithmetic_parse_terminal(s + 1, max_length - 1, name, flags, special, value, end_ptr);
            if (ret != 0) {
                return ret;
            }

            if (*name != NULL) {
                free(*name);
                return WRDE_SYNTAX;
            }

            *value = !*value;
            return 0;
        }
        case '-': {
            int ret = we_arithmetic_parse_terminal(s + 1, max_length - 1, name, flags, special, value, end_ptr);
            if (ret != 0) {
                return ret;
            }

            if (*name != NULL) {
                free(*name);
                return WRDE_SYNTAX;
            }

            *value = -*value;
            return 0;
        }
        case '+': {
            int ret = we_arithmetic_parse_terminal(s + 1, max_length - 1, name, flags, special, value, end_ptr);
            if (ret != 0) {
                return ret;
            }

            if (*name != NULL) {
                free(*name);
                return WRDE_SYNTAX;
            }

            *value = +*value;
            return 0;
        }
        case '\0':
            return WRDE_SYNTAX;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': {
            long result = strtol(s, (char **) end_ptr, 0);
            *value = result;
            return 0;
        }
        case '$': {
            char *expanded = NULL;
            size_t end = 0;
            switch (s[1]) {
                case '\0':
                    return WRDE_SYNTAX;
                case '{':
                case '(':
                    end = we_find_end_of_word_expansion(s, 0, max_length) + 1;
                    break;
                default:
                    end = 2; // Assume special var
                    if (isalpha(s[1]) || s[1] == '_') {
                        while (isalpha(s[end]) || s[end] == '_') {
                            end++;
                        }
                    }
            }
            if (end == 0) {
                return WRDE_SYNTAX;
            }

            char save = s[end];
            ((char *) s)[end] = '\0';

            int ret = we_expand(s, flags, &expanded, special);
            ((char *) s)[end] = save;

            if (ret == 0) {
                ret = we_unescape(&expanded);
            }

            if (ret != 0) {
                free(expanded);
                return ret;
            }

            *value = atol(expanded);
            free(expanded);
            *end_ptr = s + end;
            return 0;
        }
        default: {
            size_t end = 0;
            while (isalpha(s[end]) || s[end] == '_') {
                end++;
            }

            if (end == 0) {
                return WRDE_SYNTAX;
            }

            *end_ptr = s + end;

            char save = s[end];
            ((char *) s)[end] = '\0';

            *name = strdup(s);
            char *value_s = getenv(s);
            ((char *) s)[end] = save;

            if (!value_s) {
                if (flags & WRDE_UNDEF) {
                    free(*name);
                    return WRDE_BADVAL;
                }

                *value = 0;
                return 0;
            }

            *value = atol(value_s);
            return 0;
        }
    }
}

enum arithmetic_op {
    OP_MULT,
    OP_DIV,
    OP_MODULO,
    OP_ADD,
    OP_SUB,
    OP_SHL,
    OP_SHR,
    OP_LT,
    OP_LTE,
    OP_GT,
    OP_GTE,
    OP_EQ,
    OP_NEQ,
    OP_AND,
    OP_XOR,
    OP_OR,
    OP_LAND,
    OP_LOR,
    OP_TERNARY,
    OP_ASSIGN,
    OP_ADD_ASSIGN,
    OP_SUB_ASSIGN,
    OP_MULT_ASSIGN,
    OP_DIV_ASSIGN,
    OP_MODULO_ASSIGN,
    OP_SHL_ASSIGN,
    OP_SHR_ASSIGN,
    OP_AND_ASSIGN,
    OP_XOR_ASSIGN,
    OP_OR_ASSIGN,
    OP_COMMA,
    OP_PARANETHESIS
};

static long we_arithmetic_do_op(enum arithmetic_op op, long v1, long v2) {
    switch (op) {
        case OP_MULT:
            return v1 * v2;
        case OP_DIV:
            return v1 / v2;
        case OP_MODULO:
            return v1 % v2;
        case OP_ADD:
            return v1 + v2;
        case OP_SUB:
            return v1 - v2;
        case OP_SHL:
            return v1 << v2;
        case OP_SHR:
            return v1 >> v2;
        case OP_LT:
            return v1 < v2;
        case OP_LTE:
            return v1 <= v2;
        case OP_GT:
            return v1 > v2;
        case OP_GTE:
            return v1 >= v2;
        case OP_EQ:
            return v1 == v2;
        case OP_NEQ:
            return v1 != v2;
        case OP_AND:
            return v1 & v2;
        case OP_OR:
            return v1 | v2;
        case OP_LAND:
            return v1 && v2;
        case OP_LOR:
            return v1 || v2;
        handle_assignment:
        case OP_ASSIGN: {
            char *name = (char *) v1;

            char buf[50];
            snprintf(buf, 49, "%ld", v2);
            setenv(name, buf, 1);
            free(name);
            return v2;
        }
        case OP_ADD_ASSIGN:
            v2 = atol(getenv((char *) v1) ? getenv((char *) v1) : "0") + v2;
            goto handle_assignment;
        case OP_SUB_ASSIGN:
            v2 = atol(getenv((char *) v1) ? getenv((char *) v1) : "0") - v2;
            goto handle_assignment;
        case OP_MULT_ASSIGN:
            v2 = atol(getenv((char *) v1) ? getenv((char *) v1) : "0") * v2;
            goto handle_assignment;
        case OP_DIV_ASSIGN:
            v2 = atol(getenv((char *) v1) ? getenv((char *) v1) : "0") / v2;
            goto handle_assignment;
        case OP_MODULO_ASSIGN:
            v2 = atol(getenv((char *) v1) ? getenv((char *) v1) : "0") % v2;
            goto handle_assignment;
        case OP_SHL_ASSIGN:
            v2 = atol(getenv((char *) v1) ? getenv((char *) v1) : "0") << v2;
            goto handle_assignment;
        case OP_SHR_ASSIGN:
            v2 = atol(getenv((char *) v1) ? getenv((char *) v1) : "0") >> v2;
            goto handle_assignment;
        case OP_AND_ASSIGN:
            v2 = atol(getenv((char *) v1) ? getenv((char *) v1) : "0") & v2;
            goto handle_assignment;
        case OP_XOR_ASSIGN:
            v2 = atol(getenv((char *) v1) ? getenv((char *) v1) : "0") ^ v2;
            goto handle_assignment;
        case OP_OR_ASSIGN:
            v2 = atol(getenv((char *) v1) ? getenv((char *) v1) : "0") | v2;
            goto handle_assignment;
        case OP_COMMA:
            return ((void) v1), v2;
        case OP_PARANETHESIS:
        default:
            assert(false);
    }

    return 0;
}

static int we_arithmetic_op_precedence(enum arithmetic_op op) {
    switch (op) {
        case OP_MULT:
        case OP_DIV:
        case OP_MODULO:
            return 3;
        case OP_ADD:
        case OP_SUB:
            return 4;
        case OP_SHL:
        case OP_SHR:
            return 5;
        case OP_LT:
        case OP_LTE:
        case OP_GT:
        case OP_GTE:
            return 6;
        case OP_EQ:
        case OP_NEQ:
            return 7;
        case OP_AND:
            return 8;
        case OP_XOR:
            return 9;
        case OP_OR:
            return 10;
        case OP_LAND:
            return 11;
        case OP_LOR:
            return 12;
        case OP_TERNARY:
            return 13;
        case OP_ASSIGN:
        case OP_ADD_ASSIGN:
        case OP_SUB_ASSIGN:
        case OP_MULT_ASSIGN:
        case OP_DIV_ASSIGN:
        case OP_MODULO_ASSIGN:
        case OP_SHL_ASSIGN:
        case OP_SHR_ASSIGN:
        case OP_AND_ASSIGN:
        case OP_XOR_ASSIGN:
        case OP_OR_ASSIGN:
            return 14;
        case OP_COMMA:
            return 15;
        case OP_PARANETHESIS:
            return 16;
        default:
            assert(false);
    }

    return 0;
}

bool we_arithmetic_left_associative(enum arithmetic_op op) {
    switch (op) {
        case OP_TERNARY:
        case OP_ASSIGN:
        case OP_ADD_ASSIGN:
        case OP_SUB_ASSIGN:
        case OP_MULT_ASSIGN:
        case OP_DIV_ASSIGN:
        case OP_MODULO_ASSIGN:
        case OP_SHL_ASSIGN:
        case OP_SHR_ASSIGN:
        case OP_AND_ASSIGN:
        case OP_XOR_ASSIGN:
        case OP_OR_ASSIGN:
            return false;
        default:
            return true;
    }
}

// Takes expression of form $((s))
int we_arithmetic_expand(const char *s, size_t length, int flags, word_special_t *special, long *value) {
    const char *current = s;

    long value_stack[512];
    size_t value_stack_index = 0;
    enum arithmetic_op op_stack[512];
    size_t op_stack_index = 0;

    while (isspace(*current)) {
        current++;
    }

    while (current - s < (ptrdiff_t) length) {
        while (*current == '(' || isspace(*current)) {
            if (*current == '(') {
                op_stack[op_stack_index++] = OP_PARANETHESIS;
            }
            current++;
        }

        char *name = NULL;
        int ret = we_arithmetic_parse_terminal(current, length - (current - s), &name, flags, special, &value_stack[value_stack_index++],
                                               &current);
        if (ret != 0) {
            return ret;
        }

        while (isspace(*current)) {
            current++;
        }

        if (current - s >= (ptrdiff_t) length) {
        we_finish_arithmetic_expand_computation:
            // Name doesn't matter at this point
            free(name);

            // Pop value / op stack
            while (op_stack_index > 0) {
                assert(value_stack_index >= 2);
                value_stack[value_stack_index - 2] =
                    we_arithmetic_do_op(op_stack[--op_stack_index], value_stack[value_stack_index - 2], value_stack[value_stack_index - 1]);
                value_stack_index--;
            }
            break;
        }

    arithmetic_process_operator:
        while (*current == ')' && current - s < (ptrdiff_t) length) {
            while (op_stack_index > 0 && op_stack[op_stack_index - 1] != OP_PARANETHESIS) {
                if (value_stack_index < 2) {
                    free(name);
                    return WRDE_SYNTAX;
                }
                value_stack[value_stack_index - 2] =
                    we_arithmetic_do_op(op_stack[--op_stack_index], value_stack[value_stack_index - 2], value_stack[value_stack_index - 1]);
                value_stack_index--;
            }
            op_stack_index--;
            current++;

            while (isspace(*current)) {
                current++;
            }
        }

        if (current - s >= (ptrdiff_t) length) {
            goto we_finish_arithmetic_expand_computation;
        }

        while (isspace(*current)) {
            current++;
        }

        assert(op_stack_index <= 512);
        size_t op_size = 1;
        bool name_needed = false;
        switch (current[0]) {
            case '?':
                op_stack[op_stack_index++] = OP_TERNARY;
                break;
            case '*':
                if (current[1] == '=') {
                    if (!name) {
                        return WRDE_SYNTAX;
                    }
                    value_stack[value_stack_index - 1] = (long) name;
                    name_needed = true;
                    op_stack[op_stack_index++] = OP_MULT_ASSIGN;
                    op_size++;
                } else {
                    op_stack[op_stack_index++] = OP_MULT;
                }
                break;
            case '/':
                if (current[1] == '=') {
                    if (!name) {
                        return WRDE_SYNTAX;
                    }
                    value_stack[value_stack_index - 1] = (long) name;
                    name_needed = true;
                    op_stack[op_stack_index++] = OP_DIV_ASSIGN;
                    op_size++;
                } else {
                    op_stack[op_stack_index++] = OP_DIV;
                }
                break;
            case '%':
                if (current[1] == '=') {
                    if (!name) {
                        return WRDE_SYNTAX;
                    }
                    value_stack[value_stack_index - 1] = (long) name;
                    name_needed = true;
                    op_stack[op_stack_index++] = OP_MODULO_ASSIGN;
                    op_size++;
                } else {
                    op_stack[op_stack_index++] = OP_MODULO;
                }
                break;
            case '+':
                if (current[1] == '=') {
                    if (!name) {
                        return WRDE_SYNTAX;
                    }
                    value_stack[value_stack_index - 1] = (long) name;
                    name_needed = true;
                    op_stack[op_stack_index++] = OP_ADD_ASSIGN;
                    op_size++;
                } else {
                    op_stack[op_stack_index++] = OP_ADD;
                }
                break;
            case '-':
                if (current[1] == '=') {
                    if (!name) {
                        return WRDE_SYNTAX;
                    }
                    value_stack[value_stack_index - 1] = (long) name;
                    name_needed = true;
                    op_stack[op_stack_index++] = OP_SUB_ASSIGN;
                    op_size++;
                } else {
                    op_stack[op_stack_index++] = OP_SUB;
                }
                break;
            case '<':
                if (current[1] == '<') {
                    if (current[2] == '=') {
                        if (!name) {
                            return WRDE_SYNTAX;
                        }
                        value_stack[value_stack_index - 1] = (long) name;
                        name_needed = true;
                        op_stack[op_stack_index++] = OP_SHL_ASSIGN;
                        op_size++;
                    } else {
                        op_stack[op_stack_index++] = OP_SHL;
                    }
                    op_size++;
                } else if (current[1] == '=') {
                    op_stack[op_stack_index++] = OP_LTE;
                    op_size++;
                } else {
                    op_stack[op_stack_index++] = OP_LT;
                }
                break;
            case '>':
                if (current[1] == '>') {
                    if (current[2] == '=') {
                        if (!name) {
                            return WRDE_SYNTAX;
                        }
                        value_stack[value_stack_index - 1] = (long) name;
                        name_needed = true;
                        op_stack[op_stack_index++] = OP_SHR_ASSIGN;
                        op_size++;
                    } else {
                        op_stack[op_stack_index++] = OP_SHR;
                    }
                    op_size++;
                } else if (current[1] == '=') {
                    op_stack[op_stack_index++] = OP_GTE;
                    op_size++;
                } else {
                    op_stack[op_stack_index++] = OP_GT;
                }
                break;
            case '=':
                if (current[1] == '=') {
                    op_stack[op_stack_index++] = OP_EQ;
                    op_size++;
                } else {
                    if (!name) {
                        return WRDE_SYNTAX;
                    }
                    value_stack[value_stack_index - 1] = (long) name;
                    name_needed = true;
                    op_stack[op_stack_index++] = OP_ASSIGN;
                }
                break;
            case '!':
                if (current[1] == '=') {
                    op_stack[op_stack_index++] = OP_NEQ;
                    op_size++;
                } else {
                    free(name);
                    return WRDE_SYNTAX;
                }
                break;
            case '&':
                if (current[1] == '&') {
                    op_stack[op_stack_index++] = OP_AND;
                    op_size++;
                } else if (current[1] == '=') {
                    if (!name) {
                        return WRDE_SYNTAX;
                    }
                    value_stack[value_stack_index - 1] = (long) name;
                    name_needed = true;
                    op_stack[op_stack_index++] = OP_AND_ASSIGN;
                    op_size++;
                } else {
                    op_stack[op_stack_index++] = OP_LAND;
                }
                break;
            case '|':
                if (current[1] == '|') {
                    op_stack[op_stack_index++] = OP_OR;
                    op_size++;
                } else if (current[1] == '=') {
                    if (!name) {
                        return WRDE_SYNTAX;
                    }
                    value_stack[value_stack_index - 1] = (long) name;
                    name_needed = true;
                    op_stack[op_stack_index++] = OP_OR_ASSIGN;
                    op_size++;
                } else {
                    op_stack[op_stack_index++] = OP_LOR;
                }
                break;
            case '^':
                if (current[1] == '=') {
                    if (!name) {
                        return WRDE_SYNTAX;
                    }
                    value_stack[value_stack_index - 1] = (long) name;
                    name_needed = true;
                    op_stack[op_stack_index++] = OP_XOR_ASSIGN;
                    op_size++;
                } else {
                    op_stack[op_stack_index++] = OP_XOR;
                }
                break;
            case ',':
                op_stack[op_stack_index++] = OP_COMMA;
                break;
            default:
                free(name);
                return WRDE_SYNTAX;
        }

        current += op_size;

        while (isspace(*current)) {
            current++;
        }

        if (!name_needed) {
            free(name);
        }
        name = NULL;

        // Consider precendence
        while (op_stack_index >= 2 &&
               ((we_arithmetic_op_precedence(op_stack[op_stack_index - 2]) < we_arithmetic_op_precedence(op_stack[op_stack_index - 1])) ||
                (we_arithmetic_left_associative(op_stack[op_stack_index - 2]) &&
                 we_arithmetic_op_precedence(op_stack[op_stack_index - 2]) == we_arithmetic_op_precedence(op_stack[op_stack_index - 1])))) {
            value_stack_index -= 2;
            value_stack[value_stack_index] =
                we_arithmetic_do_op(op_stack_index[op_stack - 2], value_stack[value_stack_index], value_stack[value_stack_index + 1]);
            value_stack_index++;
            op_stack_index -= 2;
            op_stack[op_stack_index] = op_stack[op_stack_index + 1];
            op_stack_index++;
        }

        if (op_stack[op_stack_index - 1] == OP_TERNARY) {
            op_stack_index--;
            long lhs_of_ternary = value_stack[--value_stack_index];

            if (*current == ':') {
                return WRDE_SYNTAX;
            }

            int count = 1;
            const char *start = current;
            while (current - s < (ptrdiff_t) length && count > 0) {
                switch (*current) {
                    case '$':
                        if (current[1] != '(' && current[1] != '{') {
                            break;
                        }
                        // fall-through
                    case '`': {
                        size_t end = we_find_end_of_word_expansion(current, 0, length - (current - s));
                        if (end == 0) {
                            return WRDE_SYNTAX;
                        }

                        current += end;
                        break;
                    }
                    case '?':
                        count++;
                        break;
                    case ':':
                        count--;
                        break;
                    default:
                        break;
                }

                current++;
            }

            if (count > 0) {
                return WRDE_SYNTAX;
            }

            assert(current[-1] == ':');

            const char *end_after_ternary = current;
            int lparen_count = 0;
            while (end_after_ternary - s < (ptrdiff_t) length) {
                switch (*end_after_ternary) {
                    case '$':
                        if (current[1] != '(' && current[1] != '{') {
                            break;
                        }
                        // fall-through
                    case '`': {
                        int ret = we_find_end_of_word_expansion(current, 0, length - (start - s));
                        if (ret < 0) {
                            return WRDE_SYNTAX;
                        }
                        break;
                    }
                    case '(':
                        lparen_count++;
                        break;
                    case ')':
                        if (lparen_count > 0) {
                            lparen_count--;
                            break;
                        }
                        // fall-through
                    case ',':
                        goto found_ternary_end;
                }
                end_after_ternary++;
            }

        found_ternary_end:
            if (lhs_of_ternary) {
                int ret = we_arithmetic_expand(start, current - 1 - start, flags, special, &value_stack[value_stack_index++]);
                if (ret < 0) {
                    return ret;
                }
            } else {
                int ret = we_arithmetic_expand(current, length - (current - s), flags, special, &value_stack[value_stack_index++]);
                if (ret < 0) {
                    return ret;
                }
            }

            current = end_after_ternary;
            if (current - s >= (ptrdiff_t) length) {
                goto we_finish_arithmetic_expand_computation;
            }

            goto arithmetic_process_operator;
        }
    }

    assert(op_stack_index == 0);
    assert(value_stack_index == 1);
    *value = value_stack[--value_stack_index];
    return 0;
}

int we_expand(const char *s, int flags, char **expanded, word_special_t *special) {
    size_t len = WE_STR_BUF_DEFAULT;
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
                // NOTE: Still does expansion if in_d_quotes
                if (prev_was_backslash || in_s_quotes) {
                    break;
                }

                if (s[i + 1] == '(') {
                    // Arithmetic Expansion
                    if (s[i + 2] == '(') {
                        const char *to_expand = s + i;
                        i = we_find_end_of_word_expansion(s, i, input_len);
                        if (i == 0) {
                            free(*expanded);
                            return WRDE_SYNTAX;
                        }

                        long value = 0;
                        int ret = we_arithmetic_expand(to_expand + 3, (s + i - 2) - (to_expand + 3) + 1, flags, special, &value);
                        if (ret != 0) {
                            free(*expanded);
                            return ret;
                        }

                        char result[50];
                        snprintf(result, 49, "%ld", value);
                        if (!we_append(expanded, result, strlen(result), &len)) {
                            return WRDE_NOSPACE;
                        }

                        prev_was_backslash = false;
                        continue;
                    }

                    // Command sub
                    const char *to_expand = s + i + 2;
                    i = we_find_end_of_word_expansion(s, i, input_len);
                    if (i == 0) {
                        free(*expanded);
                        return WRDE_SYNTAX;
                    }

                    char save = s[i];
                    ((char *) s)[i] = '\0';

                    int ret = we_command_subst(to_expand, flags, expanded, &len, special);

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
                    int ret = we_param_expand(s + i, end - i + 1, flags, special, &result, in_d_quotes);
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
                        if (in_d_quotes && special->position_args_size > 1) {
                            goto normal_var;
                        }
                        // fall-through
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

                if (to_read == 0) {
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

                int ret = we_command_subst(to_expand, flags, expanded, &len, special);
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

static int we_split(char *s, char *split_on, wordexp_t *we, int flags) {
    size_t first_field_index = we->we_wordc;
    {
        bool prev_was_blackslash = false;
        bool in_d_quotes = false;
        bool in_s_quotes = false;

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

        if (!we->we_wordv[first_field_index]) {
            return 0;
        }
    }

    for (size_t i = first_field_index; i < we->we_wordc; i++) {
        char *current_string = we->we_wordv[i];
        bool should_free_current_string = false;

        bool in_d_quotes = false;
        bool in_s_quotes = false;
        bool prev_was_backslash = false;
        for (size_t j = 0; current_string[j] != '\0'; j++) {
            switch (current_string[j]) {
                case '\\':
                    if (!in_s_quotes) {
                        prev_was_backslash = !prev_was_backslash;
                        continue;
                    }
                    break;
                case '\'':
                    if (!in_d_quotes && !prev_was_backslash) {
                        in_s_quotes = !in_s_quotes;
                    }
                    break;
                case '"':
                    if (!in_s_quotes && !prev_was_backslash) {
                        in_d_quotes = !in_d_quotes;
                    }
                    break;
                case '$':
                    if (in_d_quotes && !in_s_quotes && !prev_was_backslash && current_string[j + 1] == '@' && we->we_special_vars &&
                        (flags & WRDE_SPECIAL)) {
                        current_string[j] = '\0';
                        j += 2;

                        char *first_string = malloc(j + strlen(we->we_special_vars->position_args[0]));
                        if (!first_string) {
                            return WRDE_NOSPACE;
                        }

                        strcpy(first_string, current_string);
                        strcat(first_string, we->we_special_vars->position_args[0]);
                        strcat(first_string, "\"");

                        char *last_string =
                            malloc(strlen(current_string + j + 2 +
                                          strlen(we->we_special_vars->position_args[we->we_special_vars->position_args_size - 1])));

                        strcpy(last_string, "\"");
                        strcat(last_string, we->we_special_vars->position_args[we->we_special_vars->position_args_size - 1]);

                        size_t next_j = strlen(last_string);
                        strcat(last_string, current_string + j);

                        char *first_special_arg = we->we_special_vars->position_args[0];
                        we->we_special_vars->position_args[0] = first_string;

                        char *last_special_arg = we->we_special_vars->position_args[we->we_special_vars->position_args_size - 1];
                        we->we_special_vars->position_args[we->we_special_vars->position_args_size - 1] = last_string;

                        bool ret = we_insert_quoted(we, i, &we->we_special_vars->position_args[0], we->we_special_vars->position_args_size);

                        we->we_special_vars->position_args[0] = first_special_arg;
                        we->we_special_vars->position_args[we->we_special_vars->position_args_size - 1] = last_special_arg;

                        if (!ret) {
                            return WRDE_NOSPACE;
                        }

                        j = next_j;
                        i += we->we_special_vars->position_args_size - 2;
                        should_free_current_string = true;
                    }
                    break;
                default:
                    break;
            }

            prev_was_backslash = false;
        }

        if (should_free_current_string) {
            free(current_string);
        }
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

static int we_unescape_all(wordexp_t *p, size_t start) {
    for (size_t i = start; i < p->we_wordc; i++) {
        int ret = we_unescape(&p->we_wordv[i]);
        if (ret != 0) {
            return ret;
        }
    }

    return 0;
}

static int we_glob(wordexp_t *we, size_t start) {
    for (size_t i = start; i < we->we_wordc; i++) {
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

    size_t start = 0;
    if (!(flags & WRDE_APPEND)) {
        p->we_offs = p->we_wordc = 0;
        p->we_wordv = NULL;
    } else {
        start = p->we_wordc;
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
    ret = we_split(str, split_on, p, flags);
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
        ret = we_glob(p, start);

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

    ret = we_unescape_all(p, start);

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