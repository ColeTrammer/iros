#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <edit/suggestions.h>
#include <errno.h>
#include <liim/pointers.h>
#include <pwd.h>
#include <sh/sh_lexer.h>
#include <sh/sh_parser.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "builtin.h"
#include "command.h"
#include "input.h"
#include "job.h"

enum class LineStatus { Done, Continue, EscapedNewline, Error };

TInput::InputStatus ShRepl::get_input_status(const String &input) const {
    bool prev_was_backslash = false;
    bool in_s_quotes = false;
    bool in_d_quotes = false;
    bool in_b_quotes = false;

    for (size_t i = 0; i < input.size(); i++) {
        switch (input[i]) {
            case '\\':
                if (!in_s_quotes) {
                    prev_was_backslash = !prev_was_backslash;
                }
                continue;
            case '"':
                in_d_quotes = (prev_was_backslash || in_s_quotes || in_b_quotes) ? in_d_quotes : !in_d_quotes;
                break;
            case '\'':
                in_s_quotes = (in_d_quotes || in_b_quotes) ? in_s_quotes : !in_s_quotes;
                break;
            case '`':
                in_b_quotes = (prev_was_backslash || in_d_quotes || in_s_quotes) ? in_b_quotes : !in_b_quotes;
                break;
            default:
                break;
        }

        prev_was_backslash = false;
    }

    if (prev_was_backslash) {
        return TInput::InputStatus::Incomplete;
    }

    ShLexer lexer(input.string(), input.size());
    auto lex_result = lexer.lex();
    if (!lex_result) {
        return TInput::InputStatus::Incomplete;
    }

    ShParser parser(lexer);
    auto parse_result = parser.parse();
    if (!parse_result) {
        return parser.needs_more_tokens() ? TInput::InputStatus::Incomplete : TInput::InputStatus::Finished;
    }

    return TInput::InputStatus::Finished;
}

struct suggestion {
    size_t length;
    size_t index;
    char *suggestion;
};

size_t g_command_count;

SharedPtr<String> g_line;

static size_t __cwd_size;
static char *__cwd_buffer;

void __refreshcwd() {
    if (!__cwd_size) {
        __cwd_size = PATH_MAX;
    }

    if (!__cwd_buffer) {
        __cwd_buffer = static_cast<char *>(malloc(__cwd_size));
    }

    while (!getcwd(__cwd_buffer, __cwd_size)) {
        __cwd_size *= 2;
        __cwd_buffer = static_cast<char *>(realloc(__cwd_buffer, __cwd_size));
    }
}

char *__getcwd() {
    if (!__cwd_buffer) {
        __refreshcwd();
    }

    return __cwd_buffer;
}

extern struct passwd *user_passwd;
extern struct utsname system_name;

static const char *month(int m) {
    switch (m) {
        case 1:
            return "Jan";
        case 2:
            return "Feb";
        case 3:
            return "Mar";
        case '4':
            return "Apr";
        case 5:
            return "May";
        case 6:
            return "Jun";
        case 7:
            return "Jul";
        case 8:
            return "Aug";
        case 9:
            return "Sep";
        case 10:
            return "Oct";
        case 11:
            return "Nov";
        case 12:
            return "Dec";
        default:
            assert(false);
            return nullptr;
    }
}

static const char *weekday(int d) {
    switch (d) {
        case 1:
            return "Mon";
        case 2:
            return "Tue";
        case 3:
            return "Wed";
        case 4:
            return "Thu";
        case 5:
            return "Fri";
        case 6:
            return "Sat";
        case 7:
            return "Sun";
        default:
            assert(false);
            return nullptr;
    }
}

String ShRepl::get_main_prompt() const {
    const char *PS1 = getenv("PS1");
    if (!PS1) {
        PS1 = "\\$ ";
    }

    bool prev_was_backslash = false;
    String prompt;
    for (size_t i = 0; PS1[i] != '\0'; i++) {
        char c = PS1[i];
        if (prev_was_backslash) {
            switch (c) {
                case 'a':
                    prompt += String('\a');
                    break;
                case 'd': {
                    time_t time = ::time(nullptr);
                    struct tm *date = localtime(&time);
                    prompt += String::format("%s %s %d", weekday(date->tm_wday), month(date->tm_mon), date->tm_mday);
                    break;
                }
                case 'H':
                case 'h':
                    prompt += system_name.nodename;
                    break;
                case 'n':
                    prompt += String('\n');
                    break;
                case 'r':
                    prompt += String('\r');
                    break;
                case 's':
                    prompt += "sh";
                    break;
                case 't': {
                    time_t time = ::time(nullptr);
                    struct tm *date = localtime(&time);
                    prompt += String::format("%02d:%02d:%02d", date->tm_hour, date->tm_min, date->tm_sec);
                    break;
                }
                case 'T': {
                    time_t time = ::time(nullptr);
                    struct tm *date = localtime(&time);
                    prompt += String::format("%02d:%02d:%02d", date->tm_hour % 12, date->tm_min, date->tm_sec);
                    break;
                }
                case '@': {
                    time_t time = ::time(nullptr);
                    struct tm *date = localtime(&time);
                    prompt += String::format("%02d:%02d %s", date->tm_hour % 12, date->tm_min, date->tm_hour < 12 ? "AM" : "PM");
                    break;
                }
                case 'A': {
                    time_t time = ::time(nullptr);
                    struct tm *date = localtime(&time);
                    prompt += String::format("%02d:%02d", date->tm_hour, date->tm_min);
                    break;
                }
                case 'u':
                    prompt += String::format("%s", user_passwd->pw_name);
                    break;
                case 'W':
                case 'w': {
                    char *cwd = strdup(__getcwd());
                    char *cwd_use = cwd;

                    size_t home_dir_length = strlen(user_passwd->pw_dir);
                    if (strcmp(user_passwd->pw_dir, "/") != 0 && strncmp(cwd, user_passwd->pw_dir, home_dir_length) == 0) {
                        cwd_use = cwd + home_dir_length - 1;
                        *cwd_use = '~';
                    }

                    prompt += cwd_use;
                    free(cwd);
                    break;
                }
                case '#':
                    prompt += String::format("%lu", g_command_count);
                    break;
                case '!':
                    prompt += String::format("%d", history().size());
                    break;
                case '$':
                    if (geteuid() == 0) {
                        prompt += String('#');
                    } else {
                        prompt += String('$');
                    }
                    break;
                case 'e':
                    prompt += String('\033');
                    break;
                case '[':
                case ']':
                    break;
                default:
                    prompt += String(c);
                    break;
            }

            prev_was_backslash = false;
            continue;
        }

        if (c == '\\') {
            prev_was_backslash = true;
        } else {
            prompt += String(c);
        }
    }

    return prompt;
}

static char *scandir_match_string = NULL;

static int scandir_filter(const struct dirent *d) {
    assert(scandir_match_string);
    if (d->d_name[0] == '.' && scandir_match_string[0] != '.') {
        return false;
    }
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

static struct suggestion *get_path_suggestions(char *line, int *num_suggestions, bool at_end) {
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
                init_suggestion(suggestions, (*num_suggestions) + i, 0, list[i]->d_name, at_end ? " " : "");
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

static struct suggestion *get_suggestions(char *line, int *num_suggestions, int *position, bool at_end) {
    *num_suggestions = 0;

    char *last_space = strrchr(line, ' ');
    bool is_first_word = last_space == NULL;
    if (is_first_word) {
        last_space = line - 1;
    }
    *position = (line + strlen(line)) - last_space - 1;

    char *to_match_start = last_space + 1;
    char *to_match = strdup(to_match_start);

    char *last_slash = strrchr(to_match, '/');
    char *dirname;
    char *currname;
    bool relative_to_root = false;
    if (last_slash == NULL) {
        if (is_first_word) {
            free(to_match);
            return get_path_suggestions(line, num_suggestions, at_end);
        }

        dirname = (char *) ".";
        currname = to_match;
    } else if (last_slash == to_match) {
        dirname = (char *) "/";
        currname = to_match + 1;
        relative_to_root = true;
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
        char *path;
        if (relative_to_root) {
            path = static_cast<char *>(malloc(strlen(list[i]->d_name) + 2));
            strcpy(stpcpy(path, "/"), list[i]->d_name);
        } else {
            path = static_cast<char *>(malloc(strlen(dirname) + strlen(list[i]->d_name) + 2));
            stpcpy(stpcpy(stpcpy(path, dirname), "/"), list[i]->d_name);
        }

        if (stat(path, &stat_struct)) {
            goto suggestions_skip_entry;
        }

        if (is_first_word && !(stat_struct.st_mode & S_IXUSR)) {
            goto suggestions_skip_entry;
        }

        free(path);
        init_suggestion(suggestions, (size_t) i, 0, list[i]->d_name, S_ISDIR(stat_struct.st_mode) ? "/" : at_end ? " " : "");

        if (last_slash == NULL) {
            suggestions[i].index = to_match_start - line;
        } else if (relative_to_root) {
            suggestions[i].index = to_match_start - line + 1;
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

Suggestions ShRepl::get_suggestions(const String &input, size_t position) const {
    String copy(input);
    copy[position] = '\0';

    int location;
    int suggestion_count;
    auto *suggestions = ::get_suggestions(copy.string(), &suggestion_count, &location, input.size() == position);

    Vector<String> result;
    for (int i = 0; i < suggestion_count; i++) {
        result.add({ suggestions[i].suggestion, suggestions[i].length });
    }

    free_suggestions(suggestions, suggestion_count);
    return Suggestions(location, move(result));
}

void ShRepl::did_get_input(const String &input) {
    ShLexer lexer(input.string(), input.size());
    if (!lexer.lex()) {
        return;
    }

    ShParser parser(lexer);
    if (!parser.parse()) {
        fprintf(stderr, "%s\n", parser.error_message().string());
        set_exit_status(2);
        return;
    }

    g_line = make_shared<String>(input);
    command_run(const_cast<ShValue::Program &>(parser.result().program()));
}

void ShRepl::did_begin_loop_iteration() {
    job_check_updates(true);
}

bool ShRepl::force_stop_input() const {
    return input_should_stop();
}

void ShRepl::did_end_input() {
    command_pop_position_params();
}

static String history_file() {
    char *history_file = getenv("HISTFILE");
    if (!history_file) {
        return String::format("%s/.sh_hist", user_passwd->pw_dir);
    }
    return history_file;
}

static int history_size() {
    char *hist_size = getenv("HISTSIZE");
    int history_size;
    if (!hist_size || sscanf(hist_size, "%d", &history_size) != 1) {
        return 1000;
    }
    return history_size;
}

static ShRepl *s_the;

ShRepl &ShRepl::the() {
    return *s_the;
}

ShRepl::ShRepl() : Repl(make_unique<TInput::History>(history_file(), history_size())) {
    s_the = this;
}

ShRepl::~ShRepl() {}
