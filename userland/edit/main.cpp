#include <liim/string_view.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "editor.h"
#include "terminal_panel.h"

void print_usage_and_exit(const char* s) {
    fprintf(stderr, "Usage: %s <text-file>\n", s);
    exit(2);
}

int main(int argc, char** argv) {
    int opt;
    while ((opt = getopt(argc, argv, ":")) != -1) {
        switch (opt) {
            case '?':
            case ':':
                print_usage_and_exit(*argv);
                break;
        }
    }

    if (argc - optind != 1) {
        print_usage_and_exit(*argv);
    }

    TerminalPanel panel;
    auto document = Document::create_from_file(String(argv[optind]), panel);
    if (!document) {
        return 1;
    }
    document->display();
    return 0;
}