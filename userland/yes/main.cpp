#include <liim/string.h>
#include <unistd.h>

int main(int argc, char **argv) {
    String to_print = "y\n";
    if (int i = 1; i < argc) {
        to_print = argv[i++];
        for (; i < argc; i++) {
            to_print += " ";
            to_print += argv[i];
        }
        to_print += "\n";
    }

    for (;;) {
        write(STDOUT_FILENO, to_print.string(), to_print.size());
    }

    return 0;
}
