#include <liim/vector.h>
#include <stdio.h>
#include <unistd.h>

#include "../builtin.h"
#include "../input.h"

static Vector<String> s_dir_stack;

static void print_dirs() {
    printf("%s", __getcwd());
    s_dir_stack.for_each([](const auto &dir) {
        printf("%c%s", ' ', dir.string());
    });
    printf("%c", '\n');
}

static int op_dirs(int, char **) {
    print_dirs();
    return 0;
}
SH_REGISTER_BUILTIN(dirs, op_dirs);

static int op_pushd(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <dir>\n", argv[0]);
        return 2;
    }

    if (chdir(argv[1]) < 0) {
        perror("chdir");
        return 1;
    }

    s_dir_stack.add(String(__getcwd()));
    __refreshcwd();
    print_dirs();
    return 0;
}
SH_REGISTER_BUILTIN(pushd, op_pushd);

static int op_popd(int, char **argv) {
    if (s_dir_stack.empty()) {
        fprintf(stderr, "%s: directory stack empty\n", argv[0]);
        return 1;
    }

    if (chdir(s_dir_stack.last().string()) < 0) {
        perror("chdir");
        return 1;
    }

    s_dir_stack.remove_last();
    __refreshcwd();
    print_dirs();
    return 0;
}
SH_REGISTER_BUILTIN(popd, op_popd);
