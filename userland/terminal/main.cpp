#include <app/app.h>
#include <app/box_layout.h>
#include <app/window.h>
#include <errno.h>
#include <stdio.h>
#include <sys/select.h>
#include <unistd.h>

#include "terminal_widget.h"
#include "vga_buffer.h"
#include "vga_terminal.h"

void print_usage_and_exit(const char* s) {
    fprintf(stderr, "Usage: %s [-v]\n", s);
    exit(2);
}

int main(int argc, char** argv) {
    bool graphics_mode = true;

    int opt;
    while ((opt = getopt(argc, argv, ":v")) != -1) {
        switch (opt) {
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

    if (!graphics_mode) {
        VgaBuffer vga_buffer;
        VgaTerminal vga_terminal(vga_buffer);

        int mfd = vga_terminal.master_fd();
        int mouse_fd = open("/dev/mouse", O_RDONLY);
        int kfd = open("/dev/keyboard", O_RDONLY);

        fd_set set;
        mouse_event mouse_event;
        key_event key_event;
        for (;;) {
            FD_ZERO(&set);
            FD_SET(mfd, &set);
            FD_SET(mouse_fd, &set);
            FD_SET(kfd, &set);

            int ret = select(FD_SETSIZE, &set, nullptr, nullptr, nullptr);
            if (ret == -1) {
                if (errno == EINTR) {
                    continue;
                }

                perror("terminal: select");
                return 1;
            }

            if (FD_ISSET(mouse_fd, &set)) {
                while (read(mouse_fd, &mouse_event, sizeof(mouse_event)) == sizeof(mouse_event)) {
                    vga_terminal.on_mouse_event(mouse_event);
                }
            }

            if (FD_ISSET(kfd, &set)) {
                while (read(kfd, &key_event, sizeof(key_event)) == sizeof(key_event)) {
                    vga_terminal.on_key_event(key_event);
                }
            }

            if (FD_ISSET(mfd, &set)) {
                vga_terminal.drain_master_fd();
            }

            vga_terminal.render();
        }

        return 0;
    }

    App::App app;

    auto window = App::Window::create(nullptr, 200, 200, 80 * 8 + 10, 25 * 16 + 10, "Terminal");
    auto& layout = window->set_layout<App::VerticalBoxLayout>();

    auto& terminal_widget = layout.add<TerminalWidget>();
    window->draw();
    window->set_focused_widget(terminal_widget);
    app.enter();
    return 0;
}
