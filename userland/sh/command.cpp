#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <liim/linked_list.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <wordexp.h>

#include "builtin.h"
#include "command.h"
#include "job.h"

#ifndef USERLAND_NATIVE

static word_special_t special_vars = { { (char*) "", (char*) "", (char*) "0", NULL, (char*) "", NULL, NULL, (char*) "/bin/sh" } };

static void __set_exit_status(int n) {
    free(special_vars.vals[WRDE_SPECIAL_QUEST]);
    special_vars.vals[WRDE_SPECIAL_QUEST] = (char*) malloc(4);
    sprintf(special_vars.vals[WRDE_SPECIAL_QUEST], "%d", n);
}

#else

static int last_exit_status;

static void __set_exit_status(int n) {
    last_exit_status = n;
}

#endif /* USERLAND_NATIVE */

void set_exit_status(int n) {
    assert(WIFEXITED(n) || WIFSIGNALED(n) || WIFSTOPPED(n));

    __set_exit_status(WIFEXITED(n) ? WEXITSTATUS(n) : WIFSTOPPED(n) ? 0 : (127 + WTERMSIG(n)));
}

#ifndef USERLAND_NATIVE

int get_last_exit_status() {
    return atoi(special_vars.vals[WRDE_SPECIAL_QUEST]);
}

#else
int get_last_exit_status() {
    return last_exit_status;
}

#endif /* USERLAND_NATIVE */

static bool handle_redirection(ShValue::IoRedirect& desc) {
    int flags = 0;
    switch (desc.type) {
        case ShValue::IoRedirect::Type::OutputFileNameAppend: {
            flags |= O_APPEND;
        }
        // Fall through
        case ShValue::IoRedirect::Type::InputFileName:
        case ShValue::IoRedirect::Type::OutputFileName: {
            flags |= O_CREAT | O_WRONLY | O_TRUNC;
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
        default:
            assert(false);
            break;
    }

    return true;
}

#ifndef WRDE_SPECIAL
#define WRDE_SPECIAL 0
#endif /* WRDE_SPECIAL */

// Does the command and returns the pid of the command for the caller to wait on, (returns -1 on error) (exit status if bulit in command)
static pid_t __do_simple_command(ShValue::SimpleCommand& command, ShValue::List::Combinator mode, bool* was_builtin, pid_t to_set_pgid) {
    Vector<String> strings;
    Vector<char*> args;

    bool failed = false;
    command.words.for_each([&](const StringView& s) {
        wordexp_t we;
#ifndef USERLAND_NATIVE
        we.we_special_vars = &special_vars;
#endif /* USERLAND_NATIVE */
        String w(s);
        int ret = wordexp(w.string(), &we, WRDE_SPECIAL);
        if (ret != 0) {
            failed = true;
            return;
        }

        for (size_t i = 0; i < we.we_wordc; i++) {
            strings.add(we.we_wordv[i]);
            args.add(strings.last().string());
        }

        wordfree(&we);
    });
    args.add(nullptr);

    if (failed) {
        return -1;
    }

    struct builtin_op* op = builtin_find_op(args[0]);
    bool do_builtin = false;
    if (builtin_should_run_immediately(op)) {
        *was_builtin = true;
        return builtin_do_op(op, args.vector());
    } else if (op) {
        do_builtin = true;
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

        if (do_builtin) {
            _exit(builtin_do_op(op, args.vector()));
        }
        execvp(args[0], args.vector());

    abort_command:
        perror("Shell");
        _exit(127);
    } else if (pid < 0) {
        perror("Shell");
        return -1;
    }

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

    enum PreviousState { Set, Unset };

    char* previous_value = getenv(name.string());
    PreviousState previous_state = previous_value ? Set : Unset;

    Vector<String> words_to_expand;

    bool failed = false;
    for_clause.words.for_each([&](const auto& w) {
        wordexp_t we;
#ifndef USERLAND_NATIVE
        we.we_special_vars = &special_vars;
#endif /* USERLAND_NATIVE */
        String word(w);
        int ret = wordexp(word.string(), &we, WRDE_SPECIAL);
        if (ret < 0) {
            failed = true;
        }

        for (size_t i = 0; i < we.we_wordc; i++) {
            words_to_expand.add(String(we.we_wordv[i]));
        }

        wordfree(&we);
    });

    if (failed) {
        return -1;
    }

    words_to_expand.for_each([&](const auto& w) {
        setenv(name.string(), w.string(), 1);
        do_command_list(for_clause.action);
    });

    if (previous_state == Unset) {
        unsetenv(name.string());
    } else {
        setenv(name.string(), previous_value, 1);
    }
    return 0;
}

static pid_t __do_compound_command(ShValue::CompoundCommand& command, ShValue::List::Combinator mode, bool* was_builtin, pid_t to_set_pgid,
                                   bool in_subshell) {
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
            ret = __do_if_clause(command.if_clause.value());
            break;
        case ShValue::CompoundCommand::Type::For:
            ret = __do_for_clause(command.for_clause.value());
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

        if (i != pipeline.commands.size() - 1) {
            String output = String::format("%d", fds[i * 2 + 1]);
            created_strings.add(output);
            StringView view = { &created_strings.tail()[0], &created_strings.tail()[created_strings.tail().size() - 1] };
            switch (command.type) {
                case ShValue::Command::Type::Compound:
                    command.compound_command.value().redirect_list.add(
                        ShValue::IoRedirect { STDOUT_FILENO, ShValue::IoRedirect::Type::OutputFileDescriptor, view });
                    break;
                case ShValue::Command::Type::Simple:
                    command.simple_command.value().redirect_info.add(
                        ShValue::IoRedirect { STDOUT_FILENO, ShValue::IoRedirect::Type::OutputFileDescriptor, view });
                    break;
                default:
                    assert(false);
                    break;
            }
        }

        if (i != 0) {
            String input = String::format("%d", fds[(i - 1) * 2]);
            created_strings.add(input);
            StringView view = { &created_strings.tail()[0], &created_strings.tail()[created_strings.tail().size() - 1] };
            switch (command.type) {
                case ShValue::Command::Type::Compound:
                    command.compound_command.value().redirect_list.add(
                        ShValue::IoRedirect { STDIN_FILENO, ShValue::IoRedirect::Type::InputFileDescriptor, view });
                    break;
                case ShValue::Command::Type::Simple:
                    command.simple_command.value().redirect_info.add(
                        ShValue::IoRedirect { STDIN_FILENO, ShValue::IoRedirect::Type::InputFileDescriptor, view });
                    break;
                default:
                    assert(false);
                    break;
            }
        }

        bool is_builtin = false;
        pid_t pid = -1;
        switch (command.type) {
            case ShValue::Command::Type::Compound:
                pid = __do_compound_command(command.compound_command.value(), mode, &is_builtin, pgid, pipeline.commands.size() > 1);
                break;
            case ShValue::Command::Type::Simple:
                pid = __do_simple_command(command.simple_command.value(), mode, &is_builtin, pgid);
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
                } while ((errno != EINTR && ret != -1) && !WIFEXITED(wstatus) && !WIFSIGNALED(wstatus) && !WIFSTOPPED(wstatus));

                if (ret == -1) {
                    return -1;
                }

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

            int status = get_last_exit_status();
            if ((component.combinators[j] == ShValue::ListComponent::Combinator::And && status != 0) ||
                ((component.combinators[j] == ShValue::ListComponent::Combinator::Or) && status == 0)) {
                break;
            }
        }
    }

    if (isatty(STDOUT_FILENO)) {
        tcsetattr(STDOUT_FILENO, TCSAFLUSH, &save);
    }

    return 0;
}

int command_run(ShValue::Program& program) {
    program.for_each([&](ShValue::List& list) {
        do_command_list(list);
    });

    return 0;
}

void command_init_special_vars() {
#ifndef USERLAND_NATIVE
    special_vars.vals[WRDE_SPECIAL_QUEST] = strdup("0");
    special_vars.vals[WRDE_SPECIAL_DOLLAR] = (char*) malloc(10);
    sprintf(special_vars.vals[WRDE_SPECIAL_DOLLAR], "%d", getpid());
    special_vars.vals[WRDE_SPECIAL_EXCLAM] = strdup("");
#endif /* USERLAND_NATIVE */
}
