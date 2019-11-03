#include "builtin.h"
#include "input.h"

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

enum line_status {
    DONE,
    UNFINISHED_QUOTE,
    ESCAPED_NEWLINE
};

struct suggestion {
    size_t length;
    size_t index;
    char *suggestion;
};

static char **history;
static size_t history_length;
static size_t history_max;

static struct termios saved_termios = { 0 };

void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &saved_termios);

    struct termios to_set = saved_termios;

    to_set.c_cflag |= (CS8);
    to_set.c_lflag &= ~(ECHO | ICANON | IEXTEN);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &to_set);
}

void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_termios);
}

static char *__getcwd() {
    size_t size = 50;
    char *buffer = malloc(size);
    char *cwd = getcwd(buffer, size);
    
    while (cwd == NULL) {
        free(buffer);
        size *= 2;
        buffer = malloc(size);
        cwd = getcwd(buffer, size);
    }

    return cwd;
}

static void print_ps1_prompt() {
	char *cwd = __getcwd();
	fprintf(stderr, "\033[32m%s\033[37m:\033[36m%s\033[37m$ ", "root@os_2", cwd);
	free(cwd);
}

static char *scandir_match_string = NULL;

static int scandir_filter(const struct dirent *d) {
    assert(scandir_match_string);
    return strstr(d->d_name, scandir_match_string) == d->d_name;
}

static int suggestion_compar(const void *a, const void *b) {
    return strcmp(((const struct suggestion*) a)->suggestion, ((const struct suggestion*) b)->suggestion);
}

static void init_suggestion(struct suggestion *suggestions, size_t at, size_t suggestion_index, const char *name, const char *post) {
    suggestions[at].length = strlen(name) + strlen(post);
    suggestions[at].index = suggestion_index;
    suggestions[at].suggestion = malloc(suggestions[at].length + 1);
    strcpy(suggestions[at].suggestion, name);
    strcat(suggestions[at].suggestion, post);
}

static struct suggestion *get_path_suggestions(char *line, size_t *num_suggestions) {
    char *path_var = getenv("PATH");
    if (path_var == NULL) {
        return NULL;
    }

    char *path_copy = strdup(path_var);
    char *search_path = strtok(path_copy, ":");
    struct suggestion *suggestions = NULL;
    while (search_path != NULL) {
        scandir_match_string = line;
        struct dirent **list;
        int num_found = scandir(search_path, &list, scandir_filter, alphasort);
        
        if (num_found > 0) { 
            suggestions = realloc(suggestions, ((*num_suggestions) + num_found) * sizeof(struct suggestion));

            for (int i = 0; i < num_found; i++) {
                init_suggestion(suggestions, (*num_suggestions) + i, 0, list[i]->d_name, " ");
                free(list[i]);
            }

            (*num_suggestions) += num_found;
            free(list);
        }

        search_path = strtok(NULL, ":");
    }

    // Check builtins
    struct builtin_op *builtins = get_builtins();
    for (size_t i = 0; i < NUM_BUILTINS; i++) {
        if (strstr(builtins[i].name, line) == builtins[i].name) {
            suggestions = realloc(suggestions, ((*num_suggestions) + 1) * sizeof(struct suggestion));

            init_suggestion(suggestions, *num_suggestions, 0, builtins[i].name, " ");

            (*num_suggestions)++;
        }
    }

    free(path_copy);
    qsort(suggestions, *num_suggestions, sizeof(struct suggestion), suggestion_compar);
    return suggestions;
}

static struct suggestion *get_suggestions(char *line, size_t *num_suggestions) {
    *num_suggestions = 0;

    char *last_space = strrchr(line, ' ');
    bool is_first_word = last_space == NULL;
    if (is_first_word) {
        last_space = line - 1;
    }

    char *to_match_start = last_space + 1;
    char *to_match = strdup(to_match_start);

    char *last_slash = strrchr(to_match, '/');
    char *dirname;
    char *currname;
    if (last_slash == NULL) {
        if (is_first_word) {
            free(to_match);
            return get_path_suggestions(line, num_suggestions);
        }

        dirname = ".";
        currname = to_match;
    } else {
        *last_slash = '\0';
        dirname = to_match;
        currname = last_slash + 1;
    }

    scandir_match_string = currname;
    struct dirent **list;    
    *num_suggestions = scandir(dirname, &list, scandir_filter, alphasort);
    if (*num_suggestions <= 0) {
        free(to_match);
        return NULL;
    }

    struct suggestion *suggestions = malloc(*num_suggestions * sizeof(struct suggestion));

    for (ssize_t i = 0; i < (ssize_t) *num_suggestions; i++) {
        struct stat stat_struct;
        char *path = malloc(strlen(dirname) + strlen(list[i]->d_name) + 2);
        strcpy(path, dirname);
        strcat(path, "/");
        strcat(path, list[i]->d_name);
        if (stat(path, &stat_struct)) {
            goto suggestions_skip_entry;
        }

        if (is_first_word && (!(stat_struct.st_mode & S_IXUSR) || !(S_ISREG(stat_struct.st_mode)))) {
            goto suggestions_skip_entry;
        }

        free(path);
        init_suggestion(suggestions, (size_t) i, 0, list[i]->d_name, S_ISDIR(stat_struct.st_mode) ? "/" : " ");

        if (last_slash == NULL) {
            suggestions[i].index = to_match_start - line;
        } else {
            suggestions[i].index = to_match_start - line + (currname - dirname);
        }
        free(list[i]);
        continue;
    
    suggestions_skip_entry:
        free(path);
        (*num_suggestions)--;
        memmove(list + i, list + i + 1, ((*num_suggestions) - i) * sizeof(struct dirent*));
        i--;
        continue;
    }

    free(list);
    free(to_match);
    return suggestions;
}

static void free_suggestions(struct suggestion *suggestions, size_t num_suggestions) {
    if (num_suggestions == 0 || suggestions == NULL) {
        return;
    }

    for (size_t i = 0; i < num_suggestions; i++) {
        free(suggestions[i].suggestion);
    }

    free(suggestions);
}

// NOTE: since the suggestions are already sorted alphabetically (using alphasort) on dirents, this method
//       only needs to check the first and last strings
static size_t longest_common_starting_substring_length(struct suggestion *suggestions, size_t num_suggestions) {
    struct suggestion *last = &suggestions[num_suggestions - 1];
    size_t length = 0;
    while (suggestions->suggestion[length] != '\0' && suggestions->suggestion[length] != '\0' && suggestions->suggestion[length] == last->suggestion[length]) {
        length++;
    }
    return length;
}

// Checks whether there are any open quotes or not in the line
static enum line_status get_line_status(char *line, size_t len) {
    bool prev_was_backslash = false;
    bool in_s_quotes = false;
    bool in_d_quotes = false;
    bool in_b_quotes = false;

    for (size_t i = 0; i < len; i++) {
        switch (line[i]) {
            case '\\':
                prev_was_backslash = !prev_was_backslash;
                continue;
            case '\'':
                in_s_quotes = (prev_was_backslash || in_d_quotes || in_b_quotes) ? in_s_quotes : !in_s_quotes;
                break;
            case '"':
                in_d_quotes = (prev_was_backslash || in_s_quotes || in_b_quotes) ? in_d_quotes : !in_d_quotes;
                break;
            case '`':
                in_b_quotes = (prev_was_backslash || in_s_quotes || in_d_quotes) ? in_b_quotes : !in_b_quotes;
                break;
            default:
                break;
        }

        prev_was_backslash = false;
    }

    if (prev_was_backslash) {
        return ESCAPED_NEWLINE;
    }

    if (in_s_quotes || in_d_quotes || in_b_quotes) {
        return UNFINISHED_QUOTE;
    }

    return DONE;
}

static void history_add(char *item) {
    if (history_length > 0 && strcmp(item, history[history_length - 1]) == 0) {
        return;
    }

    if (history_length == history_max) {
        free(history[0]);
        memmove(history, history + 1, (history_max - 1) * sizeof(char*));
    }

    history[history_length] = strdup(item);
    if (history_length < history_max) {
        history_length++;
    }
}

static char *get_tty_input(FILE *tty) {
	print_ps1_prompt();

    size_t buffer_max = 1024;
    size_t buffer_index = 0;
    size_t buffer_length = 0;
    size_t buffer_min_index = 0;
    char *buffer = malloc(buffer_max);

    char *line_save = NULL;
    size_t hist_index = history_length;

    int consecutive_tab_presses = 0;

    for (;;) {
        if (buffer_length + 1 >= buffer_max) {
            buffer_max += 1024;
            buffer = realloc(buffer, buffer_max);
        }

        char c;
        errno = 0;
        int ret = read(fileno(tty), &c, 1);

        if (ret == -1) {
            // We were interrupted
            if (errno = EINTR) {
                buffer_length = 0;
                break;
            } else {
                free(line_save);
                free(buffer);
                return NULL;
            }
        }

        // User pressed ^D
        if (ret == 0) {
            free(line_save);
            free(buffer);
            return NULL;
        }

        // tab autocompletion
        if (c == '\t') {
            if (buffer_index != buffer_length) {
                continue;
            }

            size_t num_suggestions = 0;
            buffer[buffer_length] = '\0'; // Ensure buffer is null terminated
            struct suggestion *suggestions = get_suggestions(buffer, &num_suggestions);
            
            if (num_suggestions == 0) {
                consecutive_tab_presses = 0;
                continue;
            } else if (num_suggestions > 1) {
                suggestions->length = longest_common_starting_substring_length(suggestions, num_suggestions);
                if (suggestions->length == 0 || memcmp(suggestions->suggestion, buffer + suggestions->index, suggestions->length) == 0) {
                    consecutive_tab_presses++;
                }

                if (consecutive_tab_presses > 1) {
                    fprintf(stderr, "%c", '\n');

                    for (size_t i = 0; i < num_suggestions; i++) {
                        fprintf(stderr, "%s ", suggestions[i].suggestion);
                    }

                    fprintf(stderr, "%c", '\n');
					print_ps1_prompt();
					fprintf(stderr, "%s", buffer);
                    goto cleanup_suggestions;
                }
            } else {
                consecutive_tab_presses = 0;
            }

            if (buffer_length + suggestions->length >= buffer_max - 1) {
                buffer_max += 1024;
                buffer = realloc(buffer, buffer_max);
            }

            if (suggestions->length == 0) {
                goto cleanup_suggestions;
            }

            memcpy(buffer + suggestions->index, suggestions->suggestion, suggestions->length);

            char f_buf[20];
            snprintf(f_buf, 20, "\033[%luD", buffer_index - suggestions->index);
            write(fileno(tty), f_buf, strlen(f_buf));

            write(fileno(tty), suggestions->suggestion, suggestions->length);
            buffer_index = buffer_length = suggestions->index + suggestions->length;              

        cleanup_suggestions:
            free_suggestions(suggestions, num_suggestions);
            continue;
        }

        consecutive_tab_presses = 0;

        // Terminal escape sequences
        if (c == '\033') {
            read(fileno(tty), &c, 1);
            if (c != '[') {
                continue;
            }

            read(fileno(tty), &c, 1);
            if (isdigit(c)) {
                char last;
                read(fileno(tty), &last, 1);
                if (c == '~') {
                    continue;
                }

                switch (c) {
                    case '3':
                        // Delete key
                        if (buffer_index < buffer_length) {
                            memmove(buffer + buffer_index, buffer + buffer_index + 1, buffer_length - buffer_index);
                            buffer[buffer_length - 1] = ' ';

                            write(fileno(tty), "\033[s", 3);
                            write(fileno(tty), buffer + buffer_index, buffer_length - buffer_index);
                            write(fileno(tty), "\033[u", 3);

                            buffer[buffer_length--] = '\0';
                        }
                        break;
                    default:
                        continue;
                }
            }

            switch (c) {
                case 'A':
                    // Up arrow
                    if (hist_index > 0) {
                        if (hist_index >= history_length) {
                            buffer[buffer_length] = '\0';
                            if (buffer_length > 0) {
                                line_save = strdup(buffer);
                            } else {
                                line_save = NULL;
                            }
                        }

                        hist_index--;

                        memset(buffer + buffer_min_index, ' ', buffer_length - buffer_min_index);

                        if (buffer_length > 0) {
                            char f_buf[20];
                            snprintf(f_buf, 20, "\033[%luD", buffer_index - buffer_min_index);
                            write(fileno(tty), f_buf, strlen(f_buf));
                        }

                        write(fileno(tty), "\033[s", 3);
                        write(fileno(tty), buffer + buffer_min_index, buffer_length - buffer_min_index);
                        write(fileno(tty), "\033[u", 3);

                        strncpy(buffer, history[hist_index], buffer_max);
                        write(fileno(tty), buffer, strlen(buffer));
                        buffer_index = buffer_length = strlen(buffer);
                        buffer_min_index = 0;
                    }
                    continue;
                case 'B':
                    // Down arrow
                    if (hist_index < history_length) {
                        hist_index++;

                        memset(buffer + buffer_min_index, ' ', buffer_length - buffer_min_index);

                        if (buffer_length > 0) {
                            char f_buf[20];
                            snprintf(f_buf, 20, "\033[%luD", buffer_index - buffer_min_index);
                            write(fileno(tty), f_buf, strlen(f_buf));
                        }

                        write(fileno(tty), "\033[s", 3);
                        write(fileno(tty), buffer + buffer_min_index, buffer_length - buffer_min_index);
                        write(fileno(tty), "\033[u", 3);

                        if (hist_index >= history_length) {
                            if (!line_save) {
                                buffer_index = buffer_min_index = buffer_length = 0;
                                continue;
                            } else {
                                strncpy(buffer, line_save, buffer_max);
                            }
                        } else {
                            strncpy(buffer, history[hist_index], buffer_max);
                        }

                        write(fileno(tty), buffer, strlen(buffer));
                        buffer_index = buffer_length = strlen(buffer);
                        buffer_min_index = 0;
                    }
                    continue;
                case 'C':
                    // Right arrow
                    if (buffer_index < buffer_length) {
                        buffer_index++;
                        write(fileno(tty), "\033[1C", 4);
                    }
                    break;
                case 'D':
                    // Left arrow
                    if (buffer_index > buffer_min_index) {
                        buffer_index--;
                        write(fileno(tty), "\033[1D", 4);
                    }
                    break;
                default:
                    continue;
            }

            continue;
        }


        // Pressed back space
        if (c  == 127) {
            if (buffer_index > buffer_min_index) {
                memmove(buffer + buffer_index - 1, buffer + buffer_index, buffer_length - buffer_index);
                buffer[buffer_length - 1] = ' ';

                buffer_index--;
                write(fileno(tty), "\033[1D\033[s", 7);
                write(fileno(tty), buffer + buffer_index, buffer_length - buffer_index);
                write(fileno(tty), "\033[u", 3);
                buffer[buffer_length--] = '\0';
            }

            continue;
        }

        // Stop once we get to a new line
        if (c == '\n') {
            switch (get_line_status(buffer, buffer_length)) {
                case UNFINISHED_QUOTE:
                    break;
                case ESCAPED_NEWLINE:
                    if (buffer_index == buffer_length) {
                        buffer_index--;
                    }
                    buffer_length--;
                    buffer[buffer_index] = '\0';
                    break;
                case DONE:
                    write(fileno(tty), "\n", 1);
                    goto tty_input_done;
                default:
                    assert(false);
                    break;
            }

            // The line was not finished
            buffer_min_index = buffer_index;
            write(fileno(tty), "\n> ", 3);
            continue;
        }

        if (isprint(c)) {
            if (buffer_index != buffer_length) {
                memmove(buffer + buffer_index + 1, buffer + buffer_index, buffer_length - buffer_index);
            }
            buffer[buffer_index] = c;
            buffer_length++;

            // Make sure to save and restore the cursor position
            write(fileno(tty), "\033[s", 3);
            write(fileno(tty), buffer + buffer_index, buffer_length - buffer_index);
            write(fileno(tty), "\033[u\033[1C", 7);
            buffer_index++;
        }
    }

tty_input_done:
    free(line_save);
    buffer[buffer_length] = '\0';
    if (buffer_length > 0) {
        history_add(buffer);
    }
    return buffer;
}

static char *get_file_input(FILE *file) {
    int sz = 1024;
    int pos = 0;
    char *buffer = malloc(sz);

    bool prev_was_backslash = false;

    for (;;) {
        int c = fgetc(file);

        /* In a comment */
        if (c == '#' && (pos == 0 || isspace(buffer[pos - 1]))) {
            c = getc(file);
            while (c != EOF && c != '\n') {
                c = fgetc(file);
            }

            break;
        }

        if (c == EOF && pos == 0) {
            free(buffer);
            return NULL;
        }

        if (c == '\n' && prev_was_backslash) {
            buffer[--pos] = '\0';
            prev_was_backslash = false;
            continue;
        }

        if (c == '\\') {
            prev_was_backslash = true;
        } else {
            prev_was_backslash = false;
        }

        if (c == EOF || c == '\n') {
            break;
        }

        buffer[pos++] = c;

        if (pos >= sz) {
            sz *= 2;
            buffer = realloc(buffer, sz);
        }
    }

    buffer[pos] = '\0';
    return buffer;
}

static char *get_string_input(struct string_input_source *source) {
    if (source->string[source->offset] == '\0') {
        return NULL;
    }

    // Copy the string and add a \n character so the shell parses it as a line
    char *fixed_command = strdup(source->string);

    // May need to split the command if the string has new lines characters
    source->offset = strlen(source->string);
    return fixed_command;
}

struct string_input_source *input_create_string_input_source(char *s) {
    struct string_input_source *source = malloc(sizeof(struct string_input_source));
    source->offset = 0;
    source->string = s;
    return source;
}

char *input_get_line(struct input_source *source) {
    switch (source->mode) {
        case INPUT_TTY:
            enable_raw_mode();
            char * res = get_tty_input(source->source.tty);
            disable_raw_mode();
            return res;
        case INPUT_FILE:
            return get_file_input(source->source.file);
        case INPUT_STRING:
            return get_string_input(source->source.string_input_source);
        default:
            fprintf(stderr, "Invalid input mode: %d\n", source->mode);
            assert(false);
            break;
    }

    return NULL;
}

void input_cleanup(struct input_source *source) {
    // Close file if necessary
    if (source->mode == INPUT_FILE) {
        fclose(source->source.file);
    } else if (source->mode == INPUT_STRING) {
        free(source->source.string_input_source);
    }
}

void init_history() {
    char *hist_size = getenv("HISTSIZE");
    if (sscanf(hist_size, "%lu", &history_max) != 1) {
        history_length = 100;
        setenv("HISTSIZE", "100", 0);
    }

    history = calloc(history_max, sizeof(char*));

    char *hist_file = getenv("HISTFILE");
    if (!hist_file) {
        return;
    }

    FILE *file = fopen(hist_file, "r");
    if (!file) {
        return;
    }

    char *line = NULL;
    size_t line_max = 0;
    while ((getline(&line, &line_max, file)) != -1) {
        line[strlen(line) - 1] = '\0'; // Remove trailing \n
        if (strlen(line) == 0) {
            continue;
        }

        history[history_length] = strdup(line);
        history_length++;
    }

    free(line);
    fclose(file);
}

void print_history() {
    for (size_t i = 0; i < history_length; i++) {
        printf("%4lu  %s\n", i, history[i]);
    }
}

void write_history() {
    char *hist_file = getenv("HISTFILE");
    if (!hist_file) {
        return;
    }

    FILE *file = fopen(hist_file, "w");
    if (!file) {
        return;
    }

    for (size_t i = 0; i < history_length; i++) {
        fprintf(file, "%s\n", history[i]);
    }

    fclose(file);
}
