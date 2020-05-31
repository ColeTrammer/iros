#pragma once

#include <liim/vector.h>
#include <kernel/hal/input.h>

#include "terminal.h"
#include "vga_buffer.h"

class Application {
public:
    Application();
    ~Application();

    int current_mfd() const;

    Terminal& current_tty() { return m_terminals[m_current_tty]; }
    const Terminal& current_tty() const { return m_terminals[m_current_tty]; }

    void switch_to(int tty_number);
    void reset_tty_with_pid(int pid);
    int run();

private:
    bool handle_mouse_event(scroll_state scroll);
    bool handle_keyboard_event(key_event event);

    int m_current_tty { -1 };
    Vector<Terminal> m_terminals;
    VgaBuffer::GraphicsContainer m_container;
};
