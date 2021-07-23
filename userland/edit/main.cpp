#include <app/application.h>
#include <app/box_layout.h>
#include <app/window.h>
#include <edit/document.h>
#include <liim/string_view.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "app_display.h"
#include "terminal_display.h"

void print_usage_and_exit(const char* s) {
    fprintf(stderr, "Usage: %s [-ig] <text-file>\n", s);
    exit(2);
}

int main(int argc, char** argv) {
    bool read_from_stdin = false;
    bool use_graphics_mode = false;
    (void) use_graphics_mode;

    int opt;
    while ((opt = getopt(argc, argv, ":ig")) != -1) {
        switch (opt) {
            case 'i':
                read_from_stdin = true;
                break;
            case 'g':
                use_graphics_mode = true;
                break;
            case '?':
            case ':':
                print_usage_and_exit(*argv);
                break;
        }
    }

    if (argc - optind > 1) {
        print_usage_and_exit(*argv);
    }

    auto make_document = [&](Edit::Display& display) -> int {
        SharedPtr<Edit::Document> document;
        if (read_from_stdin) {
            document = Edit::Document::create_from_stdin(argv[optind] ? String(argv[optind]) : String(""), display);
        } else if (argc - optind == 1) {
            document = Edit::Document::create_from_file(String(argv[optind]), display);
        } else {
            document = Edit::Document::create_empty();
        }

        if (!document) {
            return 1;
        }

        display.set_document(move(document));
        display.enter();
        return 0;
    };

    if (use_graphics_mode) {
        auto app = App::Application::create();

        auto window = App::Window::create(nullptr, 250, 250, 400, 400, "Edit");
        auto& display = window->set_main_widget<AppDisplay>();

        display.on_quit = [&] {
            app->main_event_loop().set_should_exit(true);
        };

        int ret = make_document(display);
        if (ret) {
            return ret;
        }

        app->enter();
        return 0;
    }

    TerminalDisplay display;
    return make_document(display);
}
