#include <stdio.h>
#include <unistd.h>

#include "../builtin.h"
#include "../command.h"

static void print_usage(const char* name) {
    fprintf(stderr, "Usage: %s <optstring> <name> [ARGS...]\n", name);
}

int SH_BUILTIN_MAIN(op_getopts)(int argc, char** argv) {
    if (argc < 3) {
        print_usage(*argv);
        return 2;
    }

    const char* optind_string = getenv("OPTIND");
    if (!optind_string) {
        setenv("OPTIND", "1", 0);
        optind_string = "1";
    }

#ifdef NO_SH
    int argc_to_use = 0;
    char** argv_to_use = NULL;
#else
    auto argc_to_use = command_position_params_size() + 1;
    auto argv_to_use = command_position_params();
#endif
    if (argc != 3) {
        argc_to_use = argc - 3;
        argv_to_use = argv + 3;
    }

    if (!argv_to_use) {
        print_usage(*argv);
        return 2;
    }

    const char* optstring = argv[1];
    const char* name = argv[2];

    optind = atoi(optind_string);
    int opt = getopt(argc_to_use, argv_to_use, optstring);
    setenv("OPTIND", String::format("%d", optind).string(), 1);

    if (opt == -1) {
        setenv(name, String('?').string(), 1);
        return 1;
    }

    auto opt_string = String { static_cast<char>(opt) };
    setenv(name, opt_string.string(), 1);
    if (opt == ':') {
        auto optopt_string = String { static_cast<char>(optopt) };
        setenv("OPTARG", optopt_string.string(), 1);
        return 0;
    }

    if (optarg) {
        setenv("OPTARG", optarg, 1);
    } else {
        unsetenv("OPTARG");
    }
    return 0;
}
SH_REGISTER_BUILTIN(getopts, op_getopts);
