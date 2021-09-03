#include <app/application.h>
#include <app/flex_layout_engine.h>
#include <app/window.h>
#include <edit/document.h>
#include <liim/string_view.h>
#include <stdio.h>
#include <stdlib.h>
#include <tui/application.h>
#include <unistd.h>

#include "app_display.h"
#include "terminal_display.h"
#include "terminal_status_bar.h"

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

    auto error_message = Maybe<String> {};
    auto document = [&] {
        if (read_from_stdin) {
            return Edit::Document::create_from_stdin(argv[optind] ? String(argv[optind]) : String(""), error_message);
        }
        if (argc - optind == 1) {
            return Edit::Document::create_from_file(String(argv[optind]), error_message);
        }
        return Edit::Document::create_empty();
    }();
    assert(document);

    if (use_graphics_mode) {
        auto app = App::Application::create();

        auto window = App::Window::create(nullptr, 250, 250, 400, 400, "Edit");
        auto& display = window->set_main_widget<AppDisplay>();

        display.on_quit = [&] {
            app->main_event_loop().set_should_exit(true);
        };

        display.set_document(move(document));
        if (error_message) {
            display.send_status_message(*error_message);
        }
        display.enter();
        app->enter();
        return 0;
    }

    auto app = TUI::Application::try_create();
    if (!app) {
        fprintf(stderr, "edit: standard input is not a terminal\n");
        return 1;
    }

    auto& main_widget = app->root_window().set_main_widget<TUI::Panel>();
    auto& main_layout = main_widget.set_layout_engine<App::VerticalFlexLayoutEngine>();

    auto& display_conainer = main_layout.add<TUI::Panel>();
    auto& display_layout = display_conainer.set_layout_engine<App::HorizontalFlexLayoutEngine>();

    auto& display = display_layout.add<TerminalDisplay>();

    main_layout.add<TerminalStatusBar>();

    display.set_document(move(document));
    if (error_message) {
        display.send_status_message(*error_message);
    }
    display.enter();

    app->set_use_alternate_screen_buffer(true);
    app->set_use_mouse(true);
    app->set_quit_on_control_q(false);
    app->enter();
    return 0;
}
