#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <liim/function.h>
#include <liim/hash_map.h>
#include <liim/linked_list.h>
#include <liim/pointers.h>
#include <sh/sh_lexer.h>
#include <sh/sh_parser.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#ifndef USERLAND_NATIVE
#include <wordexp.h>
#else
#include "../../libs/libc/include/wordexp.h"
#endif /* USERLAND_NATIVE */

#include "builtin.h"
#include "command.h"
#include "job.h"
#include "sh_state.h"

HashMap<String, String> g_aliases;
HashMap<String, FunctionBody> g_functions;
extern SharedPtr<String> g_line;

static int do_command_subst(char* s);

static int loop_depth_count = 0;
static int break_count = 0;
static int continue_count = 0;
static int exec_depth_count = 0; // For ., source, or functions
static bool should_return = false;
static word_special_t special_vars = { { NULL, (char*) "", NULL, NULL, NULL }, NULL, 0, do_command_subst };

static void __set_exit_status(int n) {
    free(special_vars.vals[WRDE_SPECIAL_QUEST]);
    special_vars.vals[WRDE_SPECIAL_QUEST] = (char*) malloc(4);
    sprintf(special_vars.vals[WRDE_SPECIAL_QUEST], "%d", n);
}

void set_exit_status(int n) {
    assert(WIFEXITED(n) || WIFSIGNALED(n) || WIFSTOPPED(n));

    __set_exit_status(WIFEXITED(n) ? WEXITSTATUS(n) : WIFSTOPPED(n) ? 0 : (127 + WTERMSIG(n)));
}

int get_last_exit_status() {
    return atoi(special_vars.vals[WRDE_SPECIAL_QUEST]);
}

int get_loop_depth_count() {
    return loop_depth_count;
}

bool input_should_stop() {
    return should_return;
}

int get_exec_depth_count() {
    return exec_depth_count;
}

void inc_exec_depth_count() {
    exec_depth_count++;
}

void dec_exec_depth_count() {
    exec_depth_count--;
    should_return = false;
}

void set_should_return() {
    should_return = true;
}

static void inc_loop_depth_count() {
    loop_depth_count++;
}

static void dec_loop_depth_count() {
    loop_depth_count--;
}

void set_continue_count(int count) {
    continue_count = MIN(count, loop_depth_count);
}

void set_break_count(int count) {
    break_count = MIN(count, loop_depth_count);
}

static bool handle_redirection(ShValue::IoRedirect& desc) {
    int flags = 0;
    switch (desc.type) {
        case ShValue::IoRedirect::Type::OutputFileNameAppend:
        case ShValue::IoRedirect::Type::OutputFileNameClobber:
        case ShValue::IoRedirect::Type::InputAndOutputFileName:
        case ShValue::IoRedirect::Type::InputFileName:
        case ShValue::IoRedirect::Type::OutputFileName: {
            if (desc.type == ShValue::IoRedirect::Type::OutputFileName || desc.type == ShValue::IoRedirect::Type::OutputFileNameClobber) {
                flags |= O_CREAT | O_WRONLY | O_TRUNC;
            } else if (desc.type == ShValue::IoRedirect::Type::InputFileName) {
                flags |= O_RDONLY;
            } else if (desc.type == ShValue::IoRedirect::Type::OutputFileNameAppend) {
                flags |= O_CREAT | O_WRONLY | O_APPEND;
            } else if (desc.type == ShValue::IoRedirect::Type::InputAndOutputFileName) {
                flags |= O_RDWR | O_CREAT | O_TRUNC;
            }
            int fd = open(String(desc.rhs).string(), flags, 0644);
            if (fd == -1) {
                return false;
            }
            if (dup2(fd, desc.number) == -1) {
                return false;
            }
            if (close(fd)) {
                return false;
            }
            break;
        }
        case ShValue::IoRedirect::Type::InputFileDescriptor:
        case ShValue::IoRedirect::Type::OutputFileDescriptor: {
            int old_fd = atoi(String(desc.rhs).string());
            if (dup2(old_fd, desc.number) == -1) {
                return false;
            }
            break;
        }
        case ShValue::IoRedirect::Type::HereString:
        case ShValue::IoRedirect::Type::HereDocument: {
            String contents = String(desc.rhs);
            if (desc.type == ShValue::IoRedirect::Type::HereString) {
                contents += "\n";
            }

            if (desc.here_document_type == ShValue::IoRedirect::HereDocumentType::RemoveLeadingTabs) {
                Vector<char> chars(desc.rhs.size() + 1);
                for (size_t i = 0; i < desc.rhs.size(); i++) {
                    while (i < desc.rhs.size() && desc.rhs.start()[i] == '\t') {
                        i++;
                    }

                    while (i < desc.rhs.size() && desc.rhs.start()[i] != '\n') {
                        chars.add(desc.rhs.start()[i++]);
                    }

                    if (i < desc.rhs.size()) {
                        chars.add('\n');
                    }
                }
                chars.add('\0');
                contents = String(chars.vector());
            }

            if (desc.here_document_quoted == ShValue::IoRedirect::HereDocumentQuoted::No) {
                wordexp_t exp;
                exp.we_special_vars = &special_vars;
                int ret = wordexp(contents.string(), &exp, ShState::the().flags_for_wordexp() | WRDE_NOGLOB | WRDE_NOFS);
                if (ret != 0) {
                    wordfree(&exp);
                    return false;
                }

                assert(exp.we_wordc == 1);
                contents = String(exp.we_wordv[0]);
                wordfree(&exp);
            }

            char file_name[50];
            strcpy(file_name, "/tmp/sh_here_documentXXXXXX");
            int fd = mkstemp(file_name);
            if (fd == -1) {
                return false;
            }

            unlink(file_name); // So that is will be automatically deleted

            ssize_t ret = write(fd, contents.string(), contents.size());
            if (ret != static_cast<ssize_t>(contents.size())) {
                close(fd);
                return false;
            }

            if (dup2(fd, desc.number) == -1) {
                return false;
            }

            if (close(fd)) {
                return false;
            }

            if (lseek(desc.number, 0, SEEK_SET) == -1) {
                return false;
            }
            break;
        }
        default:
            assert(false);
            break;
    }

    return true;
}

static pid_t __do_compound_command(ShValue::CompoundCommand& command, ShValue::List::Combinator mode, bool* was_builtin, pid_t to_set_pgid,
                                   bool in_subshell);

// Does the command and returns the pid of the command for the caller to wait on, (returns -1 on error) (exit status if bulit in command)
static pid_t __do_simple_command(ShValue::SimpleCommand& command, ShValue::List::Combinator mode, bool* was_builtin, pid_t to_set_pgid) {
    auto do_assignment_word = [](const StringView& w) {
        char* eq = strchr((char*) w.start(), '=');

        char* name_raw = (char*) malloc(eq - w.start() + 1);
        memcpy(name_raw, w.start(), eq - w.start());
        name_raw[eq - w.start()] = '\0';
        auto name = String(name_raw);
        free(name_raw);

        if (w.end() == eq) {
            setenv(name.string(), "", 1);
            return;
        }

        char* word_raw = (char*) malloc(w.end() - eq + 2);
        memcpy(word_raw, eq + 1, w.end() - eq + 1);
        word_raw[w.end() - eq + 1] = '\0';

        char* expanded = nullptr;
        int ret = we_expand(word_raw, ShState::the().flags_for_wordexp(), &expanded, &special_vars);
        if (ret == 0) {
            ret = we_unescape(&expanded);
        }

        if (ret != 0) {
            free(word_raw);
            free(expanded);
            return;
        }

        setenv(name.string(), expanded, 1);
    };

    if (command.words.size() == 0) {
        command.assignment_words.for_each(do_assignment_word);
        *was_builtin = true;
        return 0;
    }

    wordexp_t we;
    we.we_offs = 0;
    we.we_wordc = 0;
    we.we_wordv = nullptr;
    we.we_special_vars = &special_vars;

    HashMap<String, bool> expanded;
    bool gone_once = false;
    Function<bool()> expand_alias = [&]() -> bool {
        String first_word(we.we_wordv[0]);
        auto* alias = g_aliases.get(first_word);
        if (alias && !expanded.get(*alias)) {
            free(we.we_wordv[0]);
            we.we_wordv[0] = strdup(alias->string());
            expanded.put(*alias, true);
        } else if (gone_once) {
            return true;
        }

        wordexp_t exp;
        exp.we_special_vars = &special_vars;
        int ret = wordexp(we.we_wordv[0], &exp, ShState::the().flags_for_wordexp());
        if (ret != 0) {
            wordfree(&exp);
            return false;
        }

        if (!we_insert(exp.we_wordv, exp.we_wordc, 0, &we)) {
            wordfree(&exp);
            return false;
        }
        wordfree(&exp);

        gone_once = true;
        return expand_alias();
    };

    if (!we_add(strdup(String(command.words[0]).string()), &we)) {
        return WRDE_NOSPACE;
    }

    if (!expand_alias()) {
        wordfree(&we);
        return 1;
    }

    for (int i = 1; i < command.words.size(); i++) {
        String w(command.words[i]);

        int ret = wordexp(w.string(), &we, ShState::the().flags_for_wordexp() | WRDE_APPEND);
        if (ret != 0) {
            return ret;
        }
    }

    auto* function_body = g_functions.get(String(we.we_wordv[0]));
    if (function_body) {
        *was_builtin = true;
        command_push_position_params(PositionArgs(we.we_wordv + 1, we.we_wordc - 1));
        inc_exec_depth_count();
        int ret = __do_compound_command(function_body->compound_command, mode, was_builtin, to_set_pgid, false);
        dec_exec_depth_count();
        command_pop_position_params();
        wordfree(&we);
        return ret;
    }

    auto* op = Sh::BuiltInManager::the().find(we.we_wordv[0]);
    if (op) {
        if (op->name() == "exec") {
            if (we.we_wordc > 1) {
                command.assignment_words.for_each(do_assignment_word);
            }

            for (int i = 0; i < command.redirect_info.size(); i++) {
                if (!handle_redirection(command.redirect_info[i])) {
                    perror("exec");
                    *was_builtin = false;
                    wordfree(&we);
                    return -1;
                }
            }
        }

        *was_builtin = true;
        int ret = op->execute(we.we_wordc, we.we_wordv);
        wordfree(&we);
        return ret;
    }

    pid_t pid = fork();

    // Child
    if (pid == 0) {
        setpgid(to_set_pgid, to_set_pgid);
        if (isatty(STDOUT_FILENO) && mode == ShValue::List::Combinator::Sequential) {
            tcsetpgrp(STDOUT_FILENO, to_set_pgid == 0 ? getpid() : to_set_pgid);
        }

        struct sigaction to_set;
        to_set.sa_handler = SIG_DFL;
        to_set.sa_flags = 0;
        sigaction(SIGINT, &to_set, NULL);

        sigset_t mask_restore;
        sigemptyset(&mask_restore);
        sigprocmask(SIG_SETMASK, &mask_restore, NULL);

        for (int i = 0; i < command.redirect_info.size(); i++) {
            if (!handle_redirection(command.redirect_info[i])) {
                goto abort_command;
            }
        }

        command.assignment_words.for_each(do_assignment_word);

        if (op) {
            _exit(op->execute(we.we_wordc, we.we_wordv));
        }

        execvp(we.we_wordv[0], we.we_wordv);

    abort_command:
        fprintf(stderr, "%s: %s\n", we.we_wordv[0], strerror(errno));
        _exit(127);
    } else if (pid < 0) {
        wordfree(&we);
        perror("Shell");
        return -1;
    }

    wordfree(&we);
    return pid;
}

static int do_command_list(ShValue::List& list);

static pid_t __do_if_clause(ShValue::IfClause& if_clause) {
    for (int i = 0; i < if_clause.conditions.size(); i++) {
        ShValue::IfClause::Condition& condition = if_clause.conditions[i];
        switch (condition.type) {
            case ShValue::IfClause::Condition::Type::If:
            case ShValue::IfClause::Condition::Type::Elif: {
                int status = do_command_list(condition.condition.value());
                if (status != 0) {
                    return status;
                }

                if (continue_count > 0 || break_count > 0 || should_return) {
                    return status;
                }

                if (get_last_exit_status() == 0) {
                    return do_command_list(condition.action);
                }
                break;
            }
            case ShValue::IfClause::Condition::Type::Else:
                return do_command_list(condition.action);
        }
    }

    return 0;
}

static pid_t __do_for_clause(ShValue::ForClause& for_clause) {
    auto name = String(for_clause.name);

    wordexp_t we;
    we.we_special_vars = &special_vars;
    for (int i = 0; i < for_clause.words.size(); i++) {
        int ret = wordexp(String(for_clause.words[i]).string(), &we, ShState::the().flags_for_wordexp() | (i != 0 ? WRDE_APPEND : 0));
        if (ret != 0) {
            wordfree(&we);
            return ret;
        }
    }

    int ret = 0;
    inc_loop_depth_count();
    for (size_t i = 0; i < we.we_wordc; i++) {
        setenv(name.string(), we.we_wordv[i], 1);
        ret = do_command_list(for_clause.action);

        if (break_count > 0) {
            break_count--;
            break;
        }

        if (continue_count > 0) {
            if (--continue_count == 0) {
                continue;
            } else {
                break;
            }
        }

        if (should_return) {
            break;
        }
    }
    dec_loop_depth_count();

    wordfree(&we);
    return ret;
}

static pid_t __do_loop_clause(ShValue::Loop& loop) {
    int ret = 0;

    inc_loop_depth_count();
    for (;;) {
        ret = do_command_list(loop.condition);
        if (ret != 0) {
            break;
        }

        if ((get_last_exit_status() == 0) ^ (loop.type == ShValue::Loop::Type::While)) {
            break;
        }

        ret = do_command_list(loop.action);
        if (ret != 0) {
            break;
        }

        if (break_count > 0) {
            break_count--;
            break;
        }

        if (continue_count > 0) {
            if (--continue_count == 0) {
                continue;
            } else {
                break;
            }
        }

        if (should_return) {
            break;
        }
    }

    dec_loop_depth_count();
    return ret;
}

static pid_t __do_case_clause(ShValue::CaseClause& case_clause) {
    char* word_expanded = NULL;
    int ret = we_expand(String(case_clause.word).string(), ShState::the().flags_for_wordexp(), &word_expanded, &special_vars);
    if (ret != 0) {
        ret = we_unescape(&word_expanded);
    }
    if (ret != 0) {
        return ret;
    }

    for (int i = 0; i < case_clause.items.size(); i++) {
        auto& case_item = case_clause.items[i];

        for (int j = 0; j < case_item.patterns.size(); j++) {
            char* pattern_expanded = NULL;
            int ret =
                we_expand(String(case_item.patterns[j]).string(), ShState::the().flags_for_wordexp(), &pattern_expanded, &special_vars);
            if (ret != 0) {
                ret = we_unescape(&pattern_expanded);
            }
            if (ret != 0) {
                free(word_expanded);
                free(pattern_expanded);
                return ret;
            }

            int result = fnmatch(pattern_expanded, word_expanded, 0);
            if (result == FNM_NOMATCH) {
                free(pattern_expanded);
                continue;
            }

            free(word_expanded);
            free(pattern_expanded);

            if (result != 0) {
                return result;
            }

            return do_command_list(case_item.action);
        }
    }

    free(word_expanded);
    set_exit_status(0);
    return 0;
}

static pid_t __do_function_definition(ShValue::FunctionDefinition& command, ShValue::List::Combinator mode, bool* was_builtin,
                                      pid_t to_set_pgid, bool in_subshell) {
    if (in_subshell) {
        // Child
        pid_t pid = fork();
        if (pid == 0) {
            setpgid(to_set_pgid, to_set_pgid);
            if (isatty(STDOUT_FILENO) && mode == ShValue::List::Combinator::Sequential) {
                tcsetpgrp(STDOUT_FILENO, to_set_pgid == 0 ? getpid() : to_set_pgid);
            }

            struct sigaction to_set;
            to_set.sa_handler = SIG_DFL;
            to_set.sa_flags = 0;
            sigaction(SIGINT, &to_set, NULL);

            sigset_t mask_restore;
            sigemptyset(&mask_restore);
            sigprocmask(SIG_SETMASK, &mask_restore, NULL);
        } else if (pid < 0) {
            perror("Shell");
            return -1;
        } else {
            *was_builtin = false;
            setpgid(pid, to_set_pgid);
            return pid;
        }
    } else {
        *was_builtin = true;
    }

    g_functions.put(String(command.name), { command.command, g_line });

    int last_status = 0;
    if (in_subshell) {
        _exit(last_status);
    } else {
        return last_status;
    }
}

static pid_t __do_compound_command(ShValue::CompoundCommand& command, ShValue::List::Combinator mode, bool* was_builtin, pid_t to_set_pgid,
                                   bool in_subshell) {
    if (command.type == ShValue::CompoundCommand::Type::Subshell) {
        in_subshell = true;
    }

    if (in_subshell || !command.redirect_list.empty()) {
        // Child
        pid_t pid = fork();
        if (pid == 0) {
            setpgid(to_set_pgid, to_set_pgid);
            if (isatty(STDOUT_FILENO) && mode == ShValue::List::Combinator::Sequential) {
                tcsetpgrp(STDOUT_FILENO, to_set_pgid == 0 ? getpid() : to_set_pgid);
            }

            struct sigaction to_set;
            to_set.sa_handler = SIG_DFL;
            to_set.sa_flags = 0;
            sigaction(SIGINT, &to_set, NULL);

            sigset_t mask_restore;
            sigemptyset(&mask_restore);
            sigprocmask(SIG_SETMASK, &mask_restore, NULL);

            command.redirect_list.for_each([&](ShValue::IoRedirect& io) {
                handle_redirection(io);
            });
        } else if (pid < 0) {
            perror("Shell");
            return -1;
        } else {
            *was_builtin = false;
            setpgid(pid, to_set_pgid);
            return pid;
        }
    } else {
        *was_builtin = true;
    }

    int ret = 0;
    switch (command.type) {
        case ShValue::CompoundCommand::Type::If:
            ret = __do_if_clause(command.clause.as<ShValue::IfClause>());
            break;
        case ShValue::CompoundCommand::Type::For:
            ret = __do_for_clause(command.clause.as<ShValue::ForClause>());
            break;
        case ShValue::CompoundCommand::Type::BraceGroup:
            ret = do_command_list(command.clause.as<ShValue::BraceGroup>());
            break;
        case ShValue::CompoundCommand::Type::Subshell:
            ret = do_command_list(command.clause.as<ShValue::Subshell>());
            break;
        case ShValue::CompoundCommand::Type::Loop:
            ret = __do_loop_clause(command.clause.as<ShValue::Loop>());
            break;
        case ShValue::CompoundCommand::Type::Case:
            ret = __do_case_clause(command.clause.as<ShValue::CaseClause>());
            break;
        default:
            assert(false);
    }

    int last_status = get_last_exit_status();
    if (in_subshell) {
        if (ret < 0) {
            _exit(127);
        }
        _exit(last_status);
    } else {
        if (ret < 0) {
            return ret;
        }
        return last_status;
    }
}

static int do_pipeline(ShValue::Pipeline& pipeline, ShValue::List::Combinator mode) {
    int fds[(pipeline.commands.size() - 1) * 2];
    for (int i = 0; i < pipeline.commands.size() - 1; i++) {
        if (pipe(fds + (i * 2))) {
            perror("sh");
            return 0;
        }
    }

    pid_t save_pgid = getpid();
    pid_t pgid = 0;
    pid_t last = 0;

    size_t num_to_wait_on = 0;
    pid_t pids[JOB_MAX_PIDS];

    int i;
    LinkedList<String> created_strings;
    for (i = 0; i < pipeline.commands.size(); i++) {
        ShValue::Command& command = pipeline.commands[i];

        if (i != 0) {
            String input = String::format("%d", fds[(i - 1) * 2]);
            created_strings.add(input);
            StringView view = { &created_strings.tail()[0], &created_strings.tail()[created_strings.tail().size() - 1] };
            switch (command.type) {
                case ShValue::Command::Type::Compound:
                    command.command.as<ShValue::CompoundCommand>().redirect_list.add(
                        ShValue::IoRedirect { STDIN_FILENO, ShValue::IoRedirect::Type::InputFileDescriptor, view });
                    break;
                case ShValue::Command::Type::Simple:
                    command.command.as<ShValue::SimpleCommand>().redirect_info.add(
                        ShValue::IoRedirect { STDIN_FILENO, ShValue::IoRedirect::Type::InputFileDescriptor, view });
                    break;
                case ShValue::Command::Type::FunctionDefinition:
                    break;
                default:
                    assert(false);
                    break;
            }
        }

        if (i != pipeline.commands.size() - 1) {
            String output = String::format("%d", fds[i * 2 + 1]);
            created_strings.add(output);
            StringView view = { &created_strings.tail()[0], &created_strings.tail()[created_strings.tail().size() - 1] };
            switch (command.type) {
                case ShValue::Command::Type::Compound:
                    command.command.as<ShValue::CompoundCommand>().redirect_list.add(
                        ShValue::IoRedirect { STDOUT_FILENO, ShValue::IoRedirect::Type::OutputFileDescriptor, view });
                    break;
                case ShValue::Command::Type::Simple:
                    command.command.as<ShValue::SimpleCommand>().redirect_info.add(
                        ShValue::IoRedirect { STDOUT_FILENO, ShValue::IoRedirect::Type::OutputFileDescriptor, view });
                    break;
                case ShValue::Command::Type::FunctionDefinition:
                    break;
                default:
                    assert(false);
                    break;
            }
        }

        bool is_builtin = false;
        pid_t pid = -1;
        switch (command.type) {
            case ShValue::Command::Type::FunctionDefinition:
                pid = __do_function_definition(command.command.as<ShValue::FunctionDefinition>(), mode, &is_builtin, pgid,
                                               pipeline.commands.size() > 1);
                break;
            case ShValue::Command::Type::Compound:
                pid = __do_compound_command(command.command.as<ShValue::CompoundCommand>(), mode, &is_builtin, pgid,
                                            pipeline.commands.size() > 1);
                break;
            case ShValue::Command::Type::Simple:
                pid = __do_simple_command(command.command.as<ShValue::SimpleCommand>(), mode, &is_builtin, pgid);
                break;
            default:
                assert(false);
                break;
        }

        if (i != pipeline.commands.size() - 1) {
            close(fds[i * 2 + 1]);
        }

        if (i != 0) {
            close(fds[(i - 1) * 2]);
        }

        if (is_builtin) {
            __set_exit_status(pid);
            continue;
        }

        if (pid == -1) {
            break;
        }

        if (pgid == 0) {
            pgid = pid;
            if (isatty(STDOUT_FILENO) && mode == ShValue::List::Combinator::Sequential) {
                tcsetpgrp(STDOUT_FILENO, pgid);
            }
        }

        setpgid(pid, pgid);

        pids[num_to_wait_on++] = pid;
        last = pid;
    }

    for (int j = (i - 1) * 2; j < (pipeline.commands.size() - 1) * 2; j += 2) {
        if (close(fds[j]) || close(fds[j + 1])) {
            return -1;
        }
    }

    if (mode == ShValue::List::Combinator::Sequential) {
        if (num_to_wait_on > 0) {
            int wstatus;
            for (size_t num_waited = 0; num_waited < num_to_wait_on; num_waited++) {
                int ret;
                do {
                    ret = waitpid(-pgid, &wstatus, WUNTRACED);
                } while ((ret == -1 && errno == EINTR) || (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus) && !WIFSTOPPED(wstatus)));

                assert(ret != -1);

                if (WIFSTOPPED(wstatus)) {
                    killpg(pgid, SIGSTOP);
                    job_add(pgid, pids, num_to_wait_on - num_waited, STOPPED);
                    printf("%c", '\n');
                    print_job(get_jid_from_pgid(pgid));
                    break; // Not sure what to set the exit status to...
                }

                if (WIFSIGNALED(wstatus) && num_waited == num_to_wait_on - 1) {
                    if (isatty(STDERR_FILENO) && WTERMSIG(wstatus) == SIGINT) {
                        fprintf(stderr, "%c", '\n');
                    } else if (isatty(STDERR_FILENO)) {
                        fprintf(stderr, "%s\n", strsignal(WTERMSIG(wstatus)));
                    }
                }

                if (ret == last) {
                    set_exit_status(wstatus);
                }
            }
        }

        if (isatty(STDOUT_FILENO)) {
            tcsetpgrp(STDOUT_FILENO, save_pgid);
        }
    } else {
        job_add(pgid, pids, i, RUNNING);
        print_job(get_jid_from_pgid(pgid));
    }

    return i == pipeline.commands.size() ? 0 : -1;
}

static int do_command_list(ShValue::List& list) {
    struct termios save;
    if (isatty(STDOUT_FILENO)) {
        tcgetattr(STDOUT_FILENO, &save);
    }

    for (int i = 0; i < list.components.size(); i++) {
        ShValue::ListComponent& component = list.components[i];
        for (int j = 0; j < component.pipelines.size(); j++) {
            int ret = do_pipeline(component.pipelines[j], list.combinators[i]);
            if (ret != 0) {
                return ret;
            }

            if (break_count > 0 || continue_count > 0 || should_return) {
                goto finish_command_list;
            }

            int status = get_last_exit_status();
            if ((component.combinators[j] == ShValue::ListComponent::Combinator::And && status != 0) ||
                ((component.combinators[j] == ShValue::ListComponent::Combinator::Or) && status == 0)) {
                break;
            }
        }
    }

finish_command_list:
    if (isatty(STDOUT_FILENO)) {
        tcsetattr(STDOUT_FILENO, TCSAFLUSH, &save);
    }

    return 0;
}

extern size_t g_command_count;

int command_run(ShValue::Program& program) {
    for (int i = 0; i < program.size(); i++) {
        auto& list = program[i];
        do_command_list(list);
        if (should_return) {
            break;
        }
    }

    g_command_count++;
    return 0;
}

static int do_command_subst(char* s) {
    ShLexer lexer(s, strlen(s));
    if (!lexer.lex()) {
        return 1;
    }

    ShParser parser(lexer);
    if (!parser.parse()) {
        return 1;
    }

    ShValue::Program pr(parser.result().program());
    return command_run(pr);
}

static Stack<PositionArgs> args_stack;

void command_push_position_params(const PositionArgs& args) {
    args_stack.push(args);
    args_stack.peek().prepend(special_vars.vals[WRDE_SPECIAL_ZERO]);
    special_vars.position_args = args_stack.peek().argv.vector() + 1;
    special_vars.position_args_size = (size_t) args_stack.peek().argc - 1;
}

void command_pop_position_params() {
    args_stack.pop();
    if (args_stack.empty()) {
        return;
    }

    special_vars.position_args = args_stack.peek().argv.vector() + 1;
    special_vars.position_args_size = (size_t) args_stack.peek().argc - 1;
}

size_t command_position_params_size() {
    return args_stack.peek().argc - 1;
}

char** command_position_params() {
    return args_stack.peek().argv.vector();
}

void command_shift_position_params_left(int amount) {
    args_stack.peek().shift(amount);

    special_vars.position_args = args_stack.peek().argv.vector() + 1;
    special_vars.position_args_size = (size_t) args_stack.peek().argc - 1;
}

void command_add_position_param(char* s) {
    args_stack.peek().add(s);

    special_vars.position_args = args_stack.peek().argv.vector() + 1;
    special_vars.position_args_size = (size_t) args_stack.peek().argc - 1;
}

void command_init_special_vars(char* arg_zero) {
    special_vars.vals[WRDE_SPECIAL_QUEST] = strdup("0");
    special_vars.vals[WRDE_SPECIAL_DOLLAR] = (char*) malloc(10);
    sprintf(special_vars.vals[WRDE_SPECIAL_DOLLAR], "%d", getpid());
    special_vars.vals[WRDE_SPECIAL_EXCLAM] = strdup("");
    special_vars.vals[WRDE_SPECIAL_ZERO] = arg_zero;

    special_vars.position_args_size = 0;
    special_vars.position_args = nullptr;
}
