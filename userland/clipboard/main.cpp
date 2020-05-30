#include <clipboard/connection.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void print_usage_and_exit(const char* s) {
    fprintf(stderr, "Usage: %s [-c text]\n", s);
    exit(2);
}

int main(int argc, char** argv) {
    int opt;
    char* text_to_set = nullptr;
    while ((opt = getopt(argc, argv, ":c:")) != -1) {
        switch (opt) {
            case 'c':
                text_to_set = optarg;
                break;
            case ':':
            case '?':
                print_usage_and_exit(*argv);
        }
    }

    Clipboard::Connection::initialize();

    if (text_to_set) {
        if (!Clipboard::Connection::the().set_clipboard_contents_to_text(String(text_to_set))) {
            fprintf(stderr, "clipboard: error setting clipboard\n");
            return 1;
        }

#ifndef __os_2__
        // Sleep to give the cliboard manager a change to see that we put something on the clipboard.
        sleep(5);
#endif /* __os_2__ */

        return 0;
    }

    auto text = Clipboard::Connection::the().get_clipboard_contents_as_text();
    if (!text.has_value()) {
        fprintf(stderr, "clipboard: unable to get clipboard text\n");
        return 1;
    }

    printf("%s\n", text.value().string());
    return 0;
}
