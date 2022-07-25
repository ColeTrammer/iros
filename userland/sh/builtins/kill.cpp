#include <liim/container/container.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "../builtin.h"
#include "../job.h"

struct SignalDescriptor {
    int number;
    const char* name;
};

constexpr static auto signals = [] {
    auto result = make_array<SignalDescriptor>({
        { SIGHUP, "HUP" },   { SIGINT, "INT" },       { SIGQUIT, "QUIT" }, { SIGBUS, "BUS" },     { SIGTRAP, "TRAP" },
        { SIGABRT, "ABRT" }, { SIGCONT, "CONT" },     { SIGFPE, "FPE" },   { SIGKILL, "KILL" },   { SIGTTIN, "TTIN" },
        { SIGTTOU, "TTOU" }, { SIGILL, "ILL" },       { SIGPIPE, "PIPE" }, { SIGALRM, "ALRM" },   { SIGTERM, "TERM" },
        { SIGSEGV, "SEGV" }, { SIGSTOP, "STOP" },     { SIGTSTP, "TSTP" }, { SIGUSR1, "USR1" },   { SIGUSR2, "USR2" },
        { SIGPOLL, "POLL" }, { SIGPROF, "PROF" },     { SIGSYS, "SYS" },   { SIGURG, "URG" },     { SIGVTALRM, "VTALRM" },
        { SIGXCPU, "XCPU" }, { SIGXFSZ, "FXSZ" },     { SIGCHLD, "CHLD" }, { SIGWINCH, "WINCH" },
#ifdef __linux__
        { SIGPWR, "PWR" },   { SIGSTKFLT, "STKFLT" },
#endif
    });
    Alg::sort(result, CompareThreeWay {}, &SignalDescriptor::number);
    return result;
}();

static Option<int> signal_from_number(const char* num) {
    auto n = atoi(num);
    if (n < 0 || n >= _NSIG) {
        return {};
    }
    return n;
}

static Option<int> signal_from_name(const char* name) {
    for (auto& signal : signals) {
        if (StringView(name) == signal.name) {
            return signal.number;
        }
    }
    return {};
}

static Option<int> signal_from_specifier(const char* specifier) {
    if (isdigit(specifier[0])) {
        return signal_from_number(specifier);
    }
    return signal_from_name(specifier);
}

static int kill_list_one(const char* specifier) {
    auto number = atoi(specifier);
    if (number == 0) {
        printf("EXIT\n");
        return 0;
    }

    if (number > 128) {
        number -= 128;
    }

    for (auto& signal : signals) {
        if (signal.number == number) {
            printf("%s\n", signal.name);
            return 0;
        }
    }
    fprintf(stderr, "kill: `%s' is an invalid signal specifier", specifier);
    return 2;
}

static int kill_list_all() {
    for (auto& iter : signals) {
        printf("%2d: %s\n", iter.number, iter.name);
    }
    return 0;
}

static void print_usage(const char* name) {
    fprintf(stderr, "Usage: %s [-l|-s SIGNAL] <PID...>\n", name);
}

int SH_BUILTIN_MAIN(op_kill)(int argc, char** argv) {
    if (argc == 1) {
        print_usage(*argv);
        return 2;
    }

    bool list = false;
    Option<int> list_signal;
    int signal = SIGTERM;

    auto first_arg = StringView { argv[1] };
    if (first_arg.starts_with("-") && first_arg != "--" && (isdigit(first_arg[1]) || isupper(first_arg[1]))) {
        auto parsed_signal = signal_from_specifier(first_arg.data() + 1);
        if (!parsed_signal) {
            fprintf(stderr, "kill: Cannot parse `%s' into a signal number\n", first_arg.data() + 1);
            return 2;
        }
        signal = *parsed_signal;
        optind = 2;
    } else {
        optind = 0;
        int opt;
        while ((opt = getopt(argc, argv, ":ls:n:")) != -1) {
            switch (opt) {
                case 'l': {
                    list = true;
                    break;
                }
                case 's': {
                    auto parsed_signal = signal_from_name(optarg);
                    if (!parsed_signal) {
                        fprintf(stderr, "kill: `%s' is not a valid signal name\n", optarg);
                        return 2;
                    }
                    signal = *parsed_signal;
                    break;
                }
                case 'n': {
                    auto parsed_signal = signal_from_number(optarg);
                    if (!parsed_signal) {
                        fprintf(stderr, "kill: `%s' is not a valid signal number\n", optarg);
                        return 2;
                    }
                    signal = *parsed_signal;
                    break;
                }
                case ':':
                case '?':
                    print_usage(*argv);
                    return 2;
            }
        }
    }

    if (list) {
        if (argc > 3) {
            print_usage(*argv);
            return 2;
        }

        if (argc == 3) {
            return kill_list_one(argv[2]);
        }
        return kill_list_all();
    }

    if (optind == argc) {
        print_usage(*argv);
        return 2;
    }

    bool any_failed = false;
    for (; optind < argc; optind++) {
        pid_t pid;
#ifdef NO_SH
        pid = atoi(argv[optind]);
#else
        if (argv[optind][0] == '%') {
            auto id = job_id(JOB_ID, atoi(argv[optind] + 1));
            pid = -get_pgid_from_id(id);
        } else {
            pid = atoi(argv[optind]);
        }
#endif

        if (kill(pid, signal)) {
            perror("kill: kill");
            any_failed = 1;
        }
    }

#ifndef NO_SH
    job_check_updates(true);
#endif
    return any_failed ? 1 : 0;
}
SH_REGISTER_BUILTIN(kill, op_kill);
