#include "input.h"
#include "builtin.h"
#include "sh_lexer.h"
#include "sh_parser.h"

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

enum class LineStatus { Done, Continue, EscapedNewline, Error };

struct suggestion {
    size_t length;
    size_t index;
    char *suggestion;
};

static char **history;
static size_t history_length;
static size_t history_max;

static struct termios saved_termios;

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
    char *buffer = (char *) malloc(size);
    char *cwd = getcwd(buffer, size);

    while (cwd == NULL) {
        free(buffer);
        size *= 2;
        buffer = (char *) malloc(size);
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
    return strcmp(((const struct suggestion *) a)->suggestion, ((const struct suggestion *) b)->suggestion);
}

static void init_suggestion(struct suggestion *suggestions, size_t at, size_t suggestion_index, const char *name, const char *post) {
    suggestions[at].length = strlen(name) + strlen(post);
    suggestions[at].index = suggestion_index;
    suggestions[at].suggestion = (char *) malloc(suggestions[at].length + 1);
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
            suggestions = (struct suggestion *) realloc(suggestions, ((*num_suggestions) + num_found) * sizeof(struct suggestion));

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
            suggestions = (struct suggestion *) realloc(suggestions, ((*num_suggestions) + 1) * sizeof(struct suggestion));

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

        dirname = (char *) ".";
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

    struct suggestion *suggestions = (struct suggestion *) malloc(*num_suggestions * sizeof(struct suggestion));

    for (ssize_t i = 0; i < (ssize_t) *num_suggestions; i++) {
        struct stat stat_struct;
        char *path = (char *) malloc(strlen(dirname) + strlen(list[i]->d_name) + 2);
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
        memmove(list + i, list + i + 1, ((*num_suggestions) - i) * sizeof(struct dirent *));
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
    while (suggestions->suggestion[length] != '\0' && suggestions->suggestion[length] != '\0' &&
           suggestions->suggestion[length] == last->suggestion[length]) {
        length++;
    }
    return length;
}

// Checks whether there are any open quotes or not in the line
static LineStatus get_line_status(char *line, size_t len, ShValue *value, bool consider_buffer_termination_to_be_end_of_line = false) {
    bool prev_was_backslash = false;

    for (size_t i = 0; i < len; i++) {
        switch (line[i]) {
            case '\\':
                prev_was_backslash = !prev_was_backslash;
                continue;
            case '\n':
                if (i == len - 1 && prev_was_backslash) {
                    return LineStatus::EscapedNewline;
                }
            default:
                break;
        }

        prev_was_backslash = false;
    }

    if (prev_was_backslash && consider_buffer_termination_to_be_end_of_line) {
        return LineStatus::EscapedNewline;
    }

    ShLexer lexer(line, len);
    auto lex_result = lexer.lex();
    if (!lex_result) {
        return LineStatus::Continue;
    }

    ShParser parser(lexer);
    auto parse_result = parser.parse();
    if (!parse_result) {
        return parser.needs_more_tokens() ? LineStatus::Continue : LineStatus::Error;
    }

    new (value) ShValue(parser.result());
    return LineStatus::Done;
}

static void history_add(char *item) {
    if (history_length > 0 && strcmp(item, history[history_length - 1]) == 0) {
        return;
    }

    if (history_length == history_max) {
        free(history[0]);
        memmove(history, history + 1, (history_max - 1) * sizeof(char *));
    }

    history[history_length] = strdup(item);
    if (history_length < history_max) {
        history_length++;
    }
}

static InputResult get_tty_input(FILE *tty, char **line, ShValue *value) {
    print_ps1_prompt();

    size_t buffer_max = 1024;
    size_t buffer_index = 0;
    size_t buffer_length = 0;
    size_t buffer_min_index = 0;
    char *buffer = (char *) malloc(buffer_max);

    char *line_save = NULL;
    size_t hist_index = history_length;

    int consecutive_tab_presses = 0;

    for (;;) {
        if (buffer_length + 1 >= buffer_max) {
            buffer_max += 1024;
            buffer = (char *) realloc(buffer, buffer_max);
        }

        char c;
        errno = 0;
        int ret = read(fileno(tty), &c, 1);

        if (ret == -1) {
            // We were interrupted
            if (errno == EINTR) {
                buffer_length = 0;
                break;
            } else {
                free(line_save);
                free(buffer);
                return InputResult::Eof;
            }
        }

        // We will never get 0 back from read, since we block for input
        assert(ret == 1);

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
                buffer = (char *) realloc(buffer, buffer_max);
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

        // Control D
        if (c == ('D' & 0x1F)) {
            if (buffer_length == 0) {
                free(line_save);
                free(buffer);
                return InputResult::Eof;
            }

            continue;
        }

        // Control W / WERASE
        if (c == ('W' & 0x1F)) {
            bool done_something = false;
            while (buffer_index > buffer_min_index &&
                   (!done_something || isalnum(buffer[buffer_index - 1]) || buffer[buffer_index - 1] == '_')) {
                done_something = isalnum(buffer[buffer_index - 1]) || buffer[buffer_index - 1] == '_';
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
                if (last == '~') {
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
                            continue;
                        default:
                            continue;
                    }
                } else if (last != ';' || (c != '1' && c != '3')) {
                    continue;
                }

                read(fileno(tty), &last, 1);
                if (last != '5') {
                    continue;
                }

                char d;
                read(fileno(tty), &d, 1);
                if (d != '~' && c == '1') {
                    switch (d) {
                        case 'C': {
                            // Control right arrow
                            size_t index = buffer_index;
                            while (index < buffer_length && !isalpha(buffer[++index]))
                                ;
                            while (index < buffer_length && isalpha(buffer[++index]))
                                ;
                            size_t delta = index - buffer_index;
                            if (delta != 0) {
                                buffer_index = index;
                                char buf[50] = { 0 };
                                snprintf(buf, 49, "\033[%luC", delta);
                                write(fileno(tty), buf, strlen(buf));
                            }
                            continue;
                        }
                        case 'D': {
                            // Control left arrow
                            size_t index = buffer_index;
                            while (index > buffer_min_index && !isalpha(buffer[--index]))
                                ;
                            while (index > buffer_min_index && isalpha(buffer[index - 1])) {
                                index--;
                            }
                            size_t delta = buffer_index - index;
                            if (delta != 0) {
                                buffer_index = index;
                                char buf[50] = { 0 };
                                snprintf(buf, 49, "\033[%luD", delta);
                                write(fileno(tty), buf, strlen(buf));
                            }
                            continue;
                        }
                        default:
                            continue;
                    }
                } else {
                    switch (c) {
                        // Control Delete
                        case '3': {
                            bool done_something = false;
                            while (buffer_index < buffer_length &&
                                   (!done_something || isalnum(buffer[buffer_index]) || buffer[buffer_index] == '_')) {
                                done_something = isalnum(buffer[buffer_index]) || buffer[buffer_index] == '_';
                                memmove(buffer + buffer_index, buffer + buffer_index + 1, buffer_length - buffer_index);
                                buffer[buffer_length - 1] = ' ';

                                write(fileno(tty), "\033[s", 3);
                                write(fileno(tty), buffer + buffer_index, buffer_length - buffer_index);
                                write(fileno(tty), "\033[u", 3);

                                buffer[buffer_length--] = '\0';
                            }
                            continue;
                        }
                        default:
                            continue;
                    }
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
                case 'H': {
                    // Home key
                    if (buffer_index > buffer_min_index) {
                        size_t delta = buffer_index - buffer_min_index;
                        buffer_index = buffer_min_index;
                        char buf[50] = { 0 };
                        snprintf(buf, 49, "\033[%luD", delta);
                        write(fileno(tty), buf, strlen(buf));
                    }
                    break;
                }
                case 'F': {
                    // End key
                    if (buffer_index < buffer_length) {
                        size_t delta = buffer_length - buffer_index;
                        buffer_index = buffer_length;
                        char buf[50] = { 0 };
                        snprintf(buf, 49, "\033[%luC", delta);
                        write(fileno(tty), buf, strlen(buf));
                    }
                    break;
                }
                default:
                    continue;
            }

            continue;
        }

        // Pressed back space
        if (c == 127) {
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
            switch (get_line_status(buffer, buffer_length, value, true)) {
                case LineStatus::Continue:
                    buffer[buffer_index++] = c;
                    buffer_length++;
                    break;
                case LineStatus::EscapedNewline:
                    if (buffer_index == buffer_length) {
                        buffer_index--;
                    }
                    buffer_length--;
                    buffer[buffer_index] = '\0';
                    break;
                case LineStatus::Done:
                    write(fileno(tty), "\n", 1);
                    goto tty_input_done;
                case LineStatus::Error:
                    write(fileno(tty), "\n", 1);
                    *line = buffer;
                    return InputResult::Error;
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

        *line = buffer;
        return InputResult::Success;
    }

    free(buffer);
    return InputResult::Empty;
}

static InputResult get_file_input(FILE *file, char **line, ShValue *value) {
    int sz = 1024;
    int pos = 0;
    char *buffer = (char *) malloc(sz);

    for (;;) {
        int c = fgetc(file);

        if (c == EOF && pos == 0) {
            free(buffer);
            return InputResult::Eof;
        }

        buffer[pos++] = c;

        if (pos >= sz) {
            sz *= 2;
            buffer = (char *) realloc(buffer, sz);
        }

        if (c == EOF || c == '\n') {
            switch (get_line_status(buffer, pos, value)) {
                case LineStatus::Continue:
                    buffer[pos++] = c;
                    break;
                case LineStatus::EscapedNewline:
                    pos--;
                    buffer[pos] = '\0';
                    break;
                case LineStatus::Done:
                    goto file_input_done;
                case LineStatus::Error:
                    *line = buffer;
                    return InputResult::Error;
                default:
                    assert(false);
                    break;
            };
        }
    }

file_input_done:
    buffer[pos] = '\0';
    *line = buffer;

    return InputResult::Success;
}

static InputResult get_string_input(struct string_input_source *source, char **line, ShValue *value) {
    int sz = 1024;
    int pos = 0;
    char *buffer = (char *) malloc(sz);

    for (;;) {
        int c = source->string[source->offset++];

        bool done = c == '\0' || source->offset >= source->max;
        if (done && pos == 0) {
            free(buffer);
            return InputResult::Eof;
        }

        buffer[pos++] = c;

        if (pos >= sz) {
            sz *= 2;
            buffer = (char *) realloc(buffer, sz);
        }

        if (done || c == '\n') {
            switch (get_line_status(buffer, pos, value)) {
                case LineStatus::Continue:
                    buffer[pos++] = c;
                    break;
                case LineStatus::EscapedNewline:
                    pos--;
                    buffer[pos] = '\0';
                    break;
                case LineStatus::Done:
                    goto file_input_done;
                case LineStatus::Error:
                    *line = buffer;
                    return InputResult::Error;
                default:
                    assert(false);
                    break;
            };
        }
    }

file_input_done:
    buffer[pos] = '\0';
    *line = buffer;

    return InputResult::Success;
}

struct string_input_source *input_create_string_input_source(char *s) {
    struct string_input_source *source = (struct string_input_source *) malloc(sizeof(struct string_input_source));
    source->offset = 0;
    source->string = s;
    source->max = strlen(s);
    return source;
}

InputResult input_get_line(struct input_source *source, char **line, ShValue *command) {
    switch (source->mode) {
        case INPUT_TTY: {
            enable_raw_mode();
            auto res = get_tty_input(source->source.tty, line, command);
            disable_raw_mode();
            return res;
        }
        case INPUT_FILE:
            return get_file_input(source->source.file, line, command);
        case INPUT_STRING:
            return get_string_input(source->source.string_input_source, line, command);
        default:
            fprintf(stderr, "Invalid input mode: %d\n", source->mode);
            assert(false);
            break;
    }

    return InputResult::Eof;
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

    history = (char **) calloc(history_max, sizeof(char *));

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
