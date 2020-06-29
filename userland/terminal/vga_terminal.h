#pragma once

#include <kernel/hal/input.h>

#include "pseudo_terminal.h"
#include "tty.h"
#include "vga_buffer.h"

class VgaTerminal {
public:
    VgaTerminal(VgaBuffer& vga_buffer);

    void render();
    void on_key_event(key_event key_event);
    void on_mouse_event(mouse_event mouse_event);

    int master_fd() const { return m_pseudo_terminal.master_fd(); }
    void drain_master_fd();

private:
    PsuedoTerminal m_pseudo_terminal;
    TTY m_tty;
    VgaBuffer& m_vga_buffer;
};
