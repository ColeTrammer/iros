#include <clipboard/connection.h>
#include <stdlib.h>
#include <unistd.h>
#include <kernel/hal/x86_64/drivers/vga.h>

#include "vga_terminal.h"

VgaTerminal::VgaTerminal(VgaBuffer& vga_buffer) : m_tty(m_pseudo_terminal), m_vga_buffer(vga_buffer) {
    m_tty.resize(m_vga_buffer.height(), m_vga_buffer.width());
}

void VgaTerminal::on_mouse_event(mouse_event event) {
    if (event.scroll_state == SCROLL_DOWN) {
        m_tty.scroll_down();
    } else if (event.scroll_state == SCROLL_UP) {
        m_tty.scroll_up();
    }
}

void VgaTerminal::on_key_event(key_event event) {
    if ((event.flags & KEY_DOWN) && (event.flags & KEY_CONTROL_ON) && (event.flags & KEY_SHIFT_ON) && (event.key == KEY_V)) {
        auto maybe_text = Clipboard::Connection::the().get_clipboard_contents_as_text();
        if (!maybe_text.has_value()) {
            return;
        }
        m_pseudo_terminal.send_clipboard_contents(maybe_text.value());
        return;
    }

    m_pseudo_terminal.handle_key_event(event.key, event.flags, event.ascii);
}

void VgaTerminal::render() {
    auto& rows = m_tty.rows();
    for (auto r = m_tty.row_offset(); r < rows.size() && r < m_tty.row_offset() + m_tty.row_count(); r++) {
        auto& row = rows[r];
        for (auto c = 0; c < row.size(); c++) {
            auto& cell = row[c];

            auto bg = cell.bg.to_vga_color().value();
            auto fg = cell.fg.to_vga_color().value();
            if (cell.inverted) {
                swap(bg, fg);
            }

            m_vga_buffer.draw(r - m_tty.row_offset(), c, bg, fg, cell.ch);
        }
    }

    int cursor_row = m_tty.cursor_row() + ((rows.size() - m_tty.row_count()) - m_tty.row_offset());
    int cursor_col = m_tty.cursor_col();
    if (m_tty.cursor_hidden() || cursor_row >= m_vga_buffer.height()) {
        m_vga_buffer.hide_cursor();
    } else {
        m_vga_buffer.show_cursor(cursor_row, cursor_col);
    }
}

void VgaTerminal::drain_master_fd() {
    char buf[BUFSIZ];
    for (;;) {
        ssize_t ret = read(m_pseudo_terminal.master_fd(), buf, sizeof(buf));
        if (ret < 0) {
            perror("terminal: read");
            exit(1);
        } else if (ret == 0) {
            break;
        }

        for (ssize_t i = 0; i < ret; i++) {
            m_tty.on_char(buf[i]);
        }
    }
}
