#include <app/application.h>
#include <app/box_layout.h>
#include <app/window.h>
#include <clipboard/connection.h>
#include <errno.h>
#include <eventloop/event.h>
#include <eventloop/input_tracker.h>
#include <liim/format.h>
#include <signal.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <tui/application.h>
#include <tui/flex_layout_engine.h>
#include <unistd.h>

#ifdef __os_2__
#include <sys/umessage.h>
#endif /* __os_2__ */

#include "terminal_panel.h"
#include "terminal_widget.h"
#include "vga_buffer.h"
#include "vga_terminal.h"

void print_usage_and_exit(const char* s) {
    fprintf(stderr, "Usage: %s [-tv]\n", s);
    exit(2);
}

int main(int argc, char** argv) {
    [[maybe_unused]] bool graphics_mode = true;
    bool terminal_mode = false;

    int opt;
    while ((opt = getopt(argc, argv, ":tv")) != -1) {
        switch (opt) {
            case 't':
                terminal_mode = true;
                break;
            case 'v':
                graphics_mode = false;
                break;
            case ':':
            case '?':
                print_usage_and_exit(*argv);
                break;
        }
    }

    if (optind != argc) {
        print_usage_and_exit(*argv);
    }

    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGWINCH, SIG_IGN);

#ifdef __os_2__
    if (!graphics_mode) {
        App::InputTracker input_tracker;

        VgaBuffer vga_buffer;
        VgaTerminal vga_terminal(vga_buffer);

        int mfd = vga_terminal.master_fd();
        int ifd = socket(AF_UMESSAGE, SOCK_DGRAM | SOCK_NONBLOCK, UMESSAGE_INPUT);

        sigset_t mask;
        sigprocmask(0, nullptr, &mask);
        sigaddset(&mask, SIGCHLD);
        sigprocmask(SIG_SETMASK, &mask, nullptr);
        sigdelset(&mask, SIGCHLD);

        struct sigaction act;
        sigemptyset(&act.sa_mask);
        act.sa_flags = 0;
        act.sa_handler = [](int) {
            for (;;) {
                int status;
                pid_t pid = waitpid(-1, &status, WNOHANG);
                if (pid <= 0) {
                    break;
                }

                assert(WIFEXITED(status) || WIFSIGNALED(status));
                exit(0);
            }
        };
        sigaction(SIGCHLD, &act, nullptr);

        fd_set set;
        for (;;) {
            FD_ZERO(&set);
            FD_SET(mfd, &set);
            FD_SET(ifd, &set);

            int ret = pselect(FD_SETSIZE, &set, nullptr, nullptr, nullptr, &mask);
            if (ret == -1) {
                if (errno == EINTR) {
                    continue;
                }

                perror("terminal: select");
                return 1;
            }

            if (FD_ISSET(ifd, &set)) {
                char buffer[400];
                ssize_t ret;
                while ((ret = read(ifd, buffer, sizeof(buffer))) > 0) {
                    auto* message = (umessage*) buffer;
                    switch (message->type) {
                        case UMESSAGE_INPUT_KEY_EVENT: {
                            auto& event = ((umessage_input_key_event*) message)->event;
                            auto key_event = input_tracker.notify_os_key_event(event.ascii, event.key, event.flags);
                            if (!key_event->generates_text()) {
                                vga_terminal.on_key_event(*key_event);
                            } else if (event.ascii) {
                                auto text_event = make_unique<App::TextEvent>(String { event.ascii });
                                vga_terminal.on_text_event(*text_event);
                            }
                            break;
                        }
                        case UMESSAGE_INPUT_MOUSE_EVENT: {
                            auto& event = ((umessage_input_mouse_event*) message)->event;
                            auto events = input_tracker.notify_os_mouse_event(event.scale_mode, event.dx, event.dy, event.dz, event.buttons,
                                                                              VGA_WIDTH, VGA_HEIGHT);
                            for (auto& ev : events) {
                                vga_terminal.on_mouse_event(*ev);
                            }
                            break;
                        }
                    }
                }
            }

            if (FD_ISSET(mfd, &set)) {
                vga_terminal.drain_master_fd();
            }

            vga_terminal.render();
        }

        return 0;
    }
#endif /* __os_2__ */

    App::EventLoop::register_signal_handler(SIGCHLD, [] {
        for (;;) {
            int status;
            pid_t pid = waitpid(-1, &status, WNOHANG);
            if (pid <= 0) {
                break;
            }

            assert(WIFEXITED(status) || WIFSIGNALED(status));
            exit(0);
        }
    });

    if (terminal_mode) {
        auto app = TUI::Application::try_create();
        if (!app) {
            error_log("terminal: stdin is not a tty");
            return 1;
        }

        auto& layout = app->set_layout_engine<TUI::FlexLayoutEngine>(TUI::FlexLayoutEngine::Direction::Horizontal);
        auto& panel = layout.add<TerminalPanel>();

        panel.make_focused();

        app->set_use_alternate_screen_buffer(true);
        app->set_use_mouse(true);
        app->enter();
        return 0;
    }

    auto app = App::Application::create();

    double opacity = 0.90;
    auto window = App::Window::create(nullptr, 200, 200, 80 * 8 + 10, 25 * 16 + 10, "Terminal", opacity != 1.0);
    window->set_main_widget<TerminalWidget>(opacity);
    app->enter();
    return 0;
}
