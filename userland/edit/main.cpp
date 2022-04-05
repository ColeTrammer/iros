#include <app/file_system_model.h>
#include <app/flex_layout_engine.h>
#include <edit/document.h>
#include <errno.h>
#include <gui/application.h>
#include <gui/window.h>
#include <liim/string_view.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <tinput/terminal_renderer.h>
#include <tui/application.h>
#include <tui/splitter.h>
#include <tui/terminal_panel.h>
#include <tui/tree_view.h>
#include <unistd.h>

#include "app_display.h"
#include "terminal_display.h"
#include "terminal_status_bar.h"

class BackgroundPanel : public TUI::Panel {
    APP_WIDGET(TUI::Panel, BackgroundPanel)

public:
    BackgroundPanel() {}

    virtual void render() override {
        auto renderer = get_renderer();

        renderer.set_origin({ 0, 0 });

        for (auto& child : children()) {
            if (child->is_base_widget()) {
                auto& widget = static_cast<const App::Widget&>(*child);
                renderer.draw_rect(widget.positioned_rect().adjusted(1), { ColorValue::White });
            }
        }

        ParentWidget::render();
    }
};

void print_usage_and_exit(const char* s) {
    error_log("Usage: {} [-ig] [text-file]", s);
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

    auto path = [&] {
        if (read_from_stdin) {
            return argv[optind] ? String(argv[optind]) : String("");
        }
        if (argc - optind == 1) {
            return String(argv[optind]);
        }
        return String("");
    }();

    bool is_directory = false;
    auto document_or_error = [&]() -> Result<SharedPtr<Edit::Document>, String> {
        if (read_from_stdin) {
            return Edit::Document::create_from_stdin(path).map_error([&](int error_code) {
                return format("Failed to open `{}': {}", path, strerror(error_code));
            });
        }
        if (argc - optind == 1) {
            auto result = Edit::Document::create_from_file(path);
            if (result.is_error()) {
                if (result.error() == EISDIR) {
                    is_directory = true;
                    return format("Opened directory: `{}'", path);
                }
                if (result.error() == ENOENT) {
                    return format("Create new file: `{}'", path);
                }
                path = "";
                return move(result).map_error([&](int error_code) {
                    return format("Failed to open `{}': {}", path, strerror(error_code));
                });
            }
            return move(result.value());
        }
        return Edit::Document::create_empty();
    }();

    auto base_file_path = [&]() -> String {
        if (read_from_stdin) {
            return "./";
        }
        if (argc - optind == 1) {
            if (auto maybe_path = Ext::Path::resolve(argv[optind])) {
                if (is_directory) {
                    return maybe_path.value().to_string();
                }
                return maybe_path.value().dirname();
            }
        }
        return "./";
    }();

    if (use_graphics_mode) {
        auto app = GUI::Application::create();

        auto window = GUI::Window::create(nullptr, 250, 250, 400, 400, "Edit");
        auto& display = window->set_main_widget<AppDisplay>();

        display.on_quit = [&] {
            app->main_event_loop().set_should_exit(true);
        };

        if (document_or_error.is_error()) {
            display.send_status_message(document_or_error.error());
            display.set_document(Edit::Document::create_default(path));
        } else {
            display.set_document(move(document_or_error.value()));
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

    auto& statusbar_layout = main_widget.set_layout_engine<App::VerticalFlexLayoutEngine>();
    statusbar_layout.set_spacing(1);

    auto& main_content_container = statusbar_layout.add<TUI::Splitter>();
    main_content_container.set_direction(App::Direction::Horizontal);

    statusbar_layout.add<TerminalStatusBar>();

    auto file_system_model = App::FileSystemModel::create(nullptr);
    auto file_system_root = file_system_model->load_initial_data(base_file_path);

    auto& sidebar = main_content_container.add_widget<TUI::TreeView>();
    sidebar.set_name_column(App::FileSystemModel::Column::Name);
    sidebar.set_model(file_system_model);
    sidebar.set_root_item(file_system_root);
    sidebar.set_layout_constraint({ 25, App::LayoutConstraint::AutoSize });
    sidebar.set_hidden(!is_directory);

    sidebar.on<App::ViewItemActivated>({}, [&](const App::ViewItemActivated& event) {
        auto& item = static_cast<App::FileSystemObject&>(*event.item());
        auto path = file_system_model->full_path(item);

        auto document_or_error = Edit::Document::create_from_file(path.to_string());
        auto& active_display = TerminalStatusBar::the().active_display();
        if (document_or_error.is_error()) {
            active_display.send_status_message(format("Failed to open `{}': {}", path, strerror(document_or_error.error())));
            return;
        }

        active_display.set_document(move(document_or_error.value()));
    });

    file_system_model->install_on_tree_view(sidebar.base());

    auto& content_container = main_content_container.add_widget<TUI::Splitter>();
    content_container.set_direction(App::Direction::Vertical);

    auto& display_container = content_container.add_widget<TUI::Splitter>();
    display_container.set_direction(App::Direction::Horizontal);

    auto& display = display_container.add_widget<TerminalDisplay>();
    display.set_show_line_numbers(true);
    display.set_word_wrap_enabled(true);

    auto& terminal_container = content_container.add_widget<TUI::Panel>();
    terminal_container.set_hidden(true);

    auto terminal = SharedPtr<TUI::TerminalPanel> {};
    auto& terminal_container_layout = terminal_container.set_layout_engine<App::HorizontalFlexLayoutEngine>();

    Function<void(TerminalDisplay&)> split_display;
    Function<void(TerminalDisplay&)> make_new_display;

    auto setup_display_handlers = [&](TerminalDisplay& display) {
        auto& key_bindings = display.key_bindings();
        key_bindings.add({ App::Key::DownArrow, App::KeyModifier::Control, App::KeyShortcut::IsMulti::Yes },
                         [&terminal_container, &terminal, &display] {
                             if (!terminal_container.hidden() && terminal) {
                                 terminal->make_focused();
                             }
                         });
        key_bindings.add({ App::Key::LeftArrow, App::KeyModifier::Control, App::KeyShortcut::IsMulti::Yes },
                         [&display_container, &display] {
                             auto prev_child = static_cast<const App::Object*>(nullptr);
                             for (auto& child : display_container.children()) {
                                 if (child.get() == &display.base()) {
                                     break;
                                 }
                                 prev_child = child.get();
                             }

                             if (prev_child && prev_child->is_base_widget()) {
                                 static_cast<App::Widget&>(const_cast<App::Object&>(*prev_child)).make_focused();
                             }
                         });
        key_bindings.add({ App::Key::RightArrow, App::KeyModifier::Control, App::KeyShortcut::IsMulti::Yes },
                         [&display_container, &display] {
                             auto prev_child = static_cast<const App::Object*>(nullptr);
                             for (int i = display_container.children().size() - 1; i >= 0; i--) {
                                 auto& child = display_container.children()[i];
                                 if (child.get() == &display.base()) {
                                     break;
                                 }
                                 prev_child = child.get();
                             }

                             if (prev_child && prev_child->is_base_widget()) {
                                 static_cast<App::Widget&>(const_cast<App::Object&>(*prev_child)).make_focused();
                             }
                         });
        key_bindings.add({ App::Key::B, App::KeyModifier::Control }, [&sidebar] {
            sidebar.set_hidden(!sidebar.hidden());
        });

        display.on<Edit::SplitDisplayEvent>({}, [&display, &split_display](auto&) {
            split_display(display);
        });
        display.on<Edit::NewDisplayEvent>({}, [&display, &make_new_display](auto&) {
            make_new_display(display);
        });
    };

    split_display = [&](TerminalDisplay& display) {
        auto& new_display = display_container.add_widget<TerminalDisplay>();
        new_display.set_document(display.document()->shared_from_this());
        new_display.set_preview_auto_complete(display.preview_auto_complete());
        new_display.set_word_wrap_enabled(display.word_wrap_enabled());
        new_display.set_show_line_numbers(display.show_line_numbers());
        new_display.cursors().restore(*display.document(), display.cursors().snapshot());
        new_display.enter();

        setup_display_handlers(new_display);
    };

    make_new_display = [&](TerminalDisplay& display) {
        auto& new_display = display_container.add_widget<TerminalDisplay>();
        new_display.set_document(Edit::Document::create_empty());
        new_display.set_preview_auto_complete(display.preview_auto_complete());
        new_display.set_word_wrap_enabled(display.word_wrap_enabled());
        new_display.set_show_line_numbers(display.show_line_numbers());
        new_display.enter();

        new_display.on<Edit::SplitDisplayEvent>({}, [&](auto&) {
            split_display(display);
        });
        setup_display_handlers(new_display);
    };

    setup_display_handlers(display);

    if (document_or_error.is_error()) {
        display.send_status_message(document_or_error.error());
        display.set_document(Edit::Document::create_default(path));
    } else {
        display.set_document(move(document_or_error.value()));
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
        terminal = terminal_container_layout.add_owned<TUI::TerminalPanel>();
        terminal->on<App::TerminalHangupEvent>({}, [&terminal, &terminal_container, &display_container](auto&) {
            terminal_container.set_hidden(true);
            terminal->remove();
            terminal = nullptr;

            TerminalStatusBar::the().active_display().make_focused();
        });

        auto& terminal_key_bindings = terminal->key_bindings();
        terminal_key_bindings.add({ App::Key::UpArrow, App::KeyModifier::Control, App::KeyShortcut::IsMulti::Yes }, [&display_container] {
            TerminalStatusBar::the().active_display().make_focused();
        });
        terminal_key_bindings.add({ App::Key::T, App::KeyModifier::Control }, [&terminal, &terminal_container, &display_container] {
            terminal_container.set_hidden(true);

            TerminalStatusBar::the().active_display().make_focused();
        });
        terminal_key_bindings.add({ App::Key::B, App::KeyModifier::Control }, [&sidebar] {
            sidebar.set_hidden(!sidebar.hidden());
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
