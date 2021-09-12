#include <app/application.h>
#include <app/flex_layout_engine.h>
#include <app/window.h>
#include <edit/document.h>
#include <liim/string_view.h>
#include <stdio.h>
#include <stdlib.h>
#include <tinput/terminal_renderer.h>
#include <tui/application.h>
#include <tui/terminal_panel.h>
#include <unistd.h>

#include "app_display.h"
#include "terminal_display.h"
#include "terminal_status_bar.h"

class BackgroundPanel : public TUI::Panel {
    APP_OBJECT(BackgroundPanel)

public:
    virtual void render() override {
        auto renderer = get_renderer();

        renderer.set_origin({ 0, 0 });

        for (auto& child : children()) {
            if (child->is_base_widget()) {
                auto& widget = static_cast<const App::Base::Widget&>(*child);
                renderer.draw_rect(widget.positioned_rect().adjusted(1), { ColorValue::White });
            }
        }

        TUI::Panel::render();
    }
};

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

    auto& root_window = app->root_window();
    auto& main_widget = root_window.set_main_widget<BackgroundPanel>();
    auto& main_layout = main_widget.set_layout_engine<App::VerticalFlexLayoutEngine>();
    main_layout.set_spacing(1);

    auto& display_conainer = main_layout.add<BackgroundPanel>();
    auto& display_layout = display_conainer.set_layout_engine<App::HorizontalFlexLayoutEngine>();
    display_layout.set_spacing(1);

    auto& display = display_layout.add<TerminalDisplay>();

    auto& terminal_container = main_layout.add<TUI::Panel>();
    terminal_container.set_hidden(true);

    auto terminal = SharedPtr<TUI::TerminalPanel> {};
    auto& terminal_container_layout = terminal_container.set_layout_engine<App::HorizontalFlexLayoutEngine>();

    main_layout.add<TerminalStatusBar>();

    display.set_document(move(document));
    if (error_message) {
        display.send_status_message(*error_message);
    }
    display.enter();

    auto global_key_bindings = App::KeyBindings {};
    global_key_bindings.add({ App::Key::T, App::KeyModifier::Control }, [&] {
        if (terminal) {
            terminal_container.set_hidden(false);
            terminal->make_focused();
            return;
        }

        terminal_container.set_hidden(false);
        terminal = terminal_container_layout.add<TUI::TerminalPanel>().shared_from_this();
        terminal->on<App::TerminalHangupEvent>({}, [&terminal, &terminal_container, &display_conainer](auto&) {
            terminal_container.set_hidden(true);
            terminal->remove();
            terminal = nullptr;

            // FIXME: handle widget removal focus changing using a stack of previously focused widgets.
            auto& first_display = display_conainer.children().first();
            if (first_display->is_base_widget()) {
                static_cast<App::Base::Widget&>(const_cast<App::Object&>(*first_display)).make_focused();
            }
        });

        auto& terminal_key_bindings = terminal->key_bindings();
        terminal_key_bindings.add({ App::Key::UpArrow, App::KeyModifier::Control, App::KeyShortcut::IsMulti::Yes }, [&display_conainer] {
            auto& first_display = display_conainer.children().first();
            if (first_display->is_base_widget()) {
                static_cast<App::Base::Widget&>(const_cast<App::Object&>(*first_display)).make_focused();
            }
        });
        terminal_key_bindings.add({ App::Key::T, App::KeyModifier::Control }, [&terminal, &terminal_container, &display_conainer] {
            terminal_container.set_hidden(true);

            // FIXME: handle widget removal focus changing using a stack of previously focused widgets.
            auto& first_display = display_conainer.children().first();
            if (first_display->is_base_widget()) {
                static_cast<App::Base::Widget&>(const_cast<App::Object&>(*first_display)).make_focused();
            }
        });

        terminal->make_focused();
    });
    root_window.set_key_bindings(move(global_key_bindings));

    app->set_use_alternate_screen_buffer(true);
    app->set_use_mouse(true);
    app->set_quit_on_control_q(false);
    app->enter();
    return 0;
}
