#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <edit/document.h>
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
#include "sh_state.h"

enum class LineStatus { Done, Continue, EscapedNewline, Error };

Repl::InputStatus ShRepl::get_input_status(const String &input) const {
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
        return Repl::InputStatus::Incomplete;
    }

    ShLexer lexer(input.string(), input.size());
    auto lex_result = lexer.lex();
    if (!lex_result) {
        return Repl::InputStatus::Incomplete;
    }

    ShParser parser(lexer);
    auto parse_result = parser.parse();
    if (!parse_result) {
        return parser.needs_more_tokens() ? Repl::InputStatus::Incomplete : Repl::InputStatus::Finished;
    }

    return Repl::InputStatus::Finished;
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

const Vector<ShRepl::Dirent> &ShRepl::ensure_directory_entries(const String &directory_in) const {
    String directory = directory_in;
    if (directory.empty()) {
        directory = "./";
    }

    auto *result = m_cached_directories.get(directory);
    if (result) {
        return *result;
    }

    dirent **raw_dirents = nullptr;
    int count = scandir(directory.string(), &raw_dirents, nullptr, nullptr);

    Vector<Dirent> dirents;
    for (int i = 0; i < count; i++) {
        auto *dirent = raw_dirents[i];

        auto full_path = String::format("%s%s", directory.string(), dirent->d_name);
        struct stat st;
        if (::stat(full_path.string(), &st) < 0) {
            continue;
        }

        auto name = String(dirent->d_name);
        if (S_ISDIR(st.st_mode)) {
            name += String('/');
        }
        dirents.add({ move(name), st.st_mode });
    }
    m_cached_directories.put(directory, move(dirents));
    return *m_cached_directories.get(directory);
}

Vector<Edit::Suggestion> ShRepl::suggest_executable(const Edit::TextIndex &start) const {
    auto *path_env = getenv("PATH");
    if (!path_env) {
        return {};
    }

    Vector<Edit::Suggestion> matches;

    String path_string = path_env;
    auto directories = path_string.split_view(':');
    for (auto &directory : directories) {
        auto directory_with_slash = String(directory);
        directory_with_slash += String('/');
        auto &dirents = ensure_directory_entries(directory_with_slash);
        for (auto &dirent : dirents) {
            if (!S_ISREG(dirent.mode) || !(dirent.mode & S_IXUSR)) {
                continue;
            }

            String name = dirent.name;
            name += String(' ');
            matches.add({ move(name), start });
        }
    }

    auto &builtins = Sh::BuiltInManager::the().builtins();
    builtins.for_each([&](auto &builtin) {
        auto builtin_name = builtin.name();
        builtin_name += String(' ');
        matches.add({ move(builtin_name), start });
    });

    return matches;
}

Vector<Edit::Suggestion> ShRepl::suggest_path_for(const String &input, const Edit::TextIndex &start, bool should_be_executable) const {
    String directory = "";
    String component = input;

    const char *last_slash = strrchr(input.string(), '/');
    if (last_slash) {
        directory = { input.string(), static_cast<size_t>(last_slash - input.string() + 1) };
        component = String(last_slash + 1);
    }

    auto &directory_entries = ensure_directory_entries(directory);
    Vector<Edit::Suggestion> matches;
    for (auto &dirent : directory_entries) {
        if (should_be_executable && !(dirent.mode & S_IXUSR)) {
            continue;
        }

        auto full_path = String::format("%s%s", directory.string(), dirent.name.string());
        auto component = dirent.name;
        if (!S_ISDIR(dirent.mode)) {
            component += String(' ');
        }
        matches.add(
            { move(component), Edit::TextIndex { start.line_index(), start.index_into_line() + static_cast<int>(directory.size()) } });
    }

    return matches;
}

Vector<Edit::Suggestion> ShRepl::get_suggestions(const Edit::Document &document, const Edit::TextIndex &cursor) const {
    auto &syntax_info = document.syntax_highlighting_info();

    auto index_for_suggestion = Edit::TextIndex { cursor.line_index(), max(cursor.index_into_line() - 1, 0) };
    auto desired_token_index = syntax_info.range_index_at_text_index(index_for_suggestion);
    auto desired_token = desired_token_index.map([&](int index) {
        return syntax_info.range(index);
    });

    if (desired_token && static_cast<ShTokenType>(desired_token->private_data()) != ShTokenType::WORD) {
        return {};
    }

    auto current_text_before_cursor = String {};
    auto start = cursor;
    if (desired_token) {
        start = desired_token->start();
        current_text_before_cursor = document.text_in_range(start, cursor);
    }

    auto should_be_executable = !desired_token || ShLexer::static_would_be_first_word_of_command(
                                                      syntax_info.size(),
                                                      [&](int index) -> ShTokenType {
                                                          return static_cast<ShTokenType>(syntax_info.range(index).private_data());
                                                      },
                                                      *desired_token_index);
    if (should_be_executable && !current_text_before_cursor.index_of('/').has_value()) {
        return suggest_executable(start);
    }
    return suggest_path_for(current_text_before_cursor, start, should_be_executable);
}

void ShRepl::did_get_input(const String &input) {
    g_line = make_shared<String>(input);
    ShLexer lexer(g_line->string(), g_line->size());
    if (!lexer.lex()) {
        return;
    }

    ShParser parser(lexer);
    if (!parser.parse()) {
        fprintf(stderr, "%s\n", parser.error_message().string());
        set_exit_status(2);
        return;
    }

    command_run(const_cast<ShValue::Program &>(parser.result().program()));
}

void ShRepl::did_begin_loop_iteration() {
    history().set_should_write_history(ShState::the().option_interactive());
    m_cached_directories.clear();
    g_line = nullptr;
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

ShRepl::ShRepl() : ReplBase(make_unique<Repl::History>(history_file(), history_size())) {
    s_the = this;
}

ShRepl::~ShRepl() {}
