#ifdef __os_2__
#include <clipboard/connection.h>
#include <stdlib.h>
#include <unistd.h>
#include <kernel/hal/x86_64/drivers/vga.h>

#include "vga_terminal.h"

VgaTerminal::VgaTerminal(VgaBuffer& vga_buffer) : m_tty(m_pseudo_terminal), m_vga_buffer(vga_buffer) {
    m_tty.set_visible_size(m_vga_buffer.height(), m_vga_buffer.width());
}

void VgaTerminal::on_mouse_event(const App::MouseEvent& event) {
    if (m_pseudo_terminal.handle_mouse_event(event)) {
        return;
    }

    if (event.z() < 0) {
        m_tty.scroll_down();
    } else if (event.z() > 0) {
        m_tty.scroll_up();
    }
}

void VgaTerminal::on_text_event(const App::TextEvent& event) {
    m_pseudo_terminal.handle_text_event(event);
}

void VgaTerminal::on_key_event(const App::KeyEvent& event) {
    if (event.key_down() && event.control_down() && event.shift_down() && event.key() == App::Key::V) {
        auto maybe_text = Clipboard::Connection::the().get_clipboard_contents_as_text();
        if (!maybe_text.has_value()) {
            return;
        }
        m_pseudo_terminal.send_clipboard_contents(maybe_text.value());
        return;
    }

    m_pseudo_terminal.handle_key_event(event);
}

void VgaTerminal::render() {
    auto& rows = m_tty.rows();
    bool cursor_is_visible = false;
    int cursor_row = -1;
    int cursor_col = -1;
    for (auto r = 0; r < m_vga_buffer.height(); r++) {
        for (auto c = 0; c < m_vga_buffer.width(); c++) {
            if (r >= m_tty.row_count() || c >= m_tty.col_count()) {
                m_vga_buffer.draw(r, c, VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY, ' ');
                continue;
            }

            auto& row = rows[r];
            auto& cell = row[c];
            if (m_tty.should_display_cursor_at_position(r, c)) {
                cursor_is_visible = true;
                cursor_row = r;
                cursor_col = c;
            }

            auto bg = cell.bg.value_or({ VGA_COLOR_BLACK }).to_vga_color().value();
            auto fg = cell.fg.value_or({ { VGA_COLOR_WHITE } }).to_vga_color().value();
            if (cell.inverted) {
                swap(bg, fg);
            }

            m_vga_buffer.draw(r, c, bg, fg, cell.ch);
        }
    }

    if (!cursor_is_visible) {
        m_vga_buffer.hide_cursor();
    } else {
        m_vga_buffer.show_cursor(cursor_row, cursor_col);
    }
}

void VgaTerminal::drain_master_fd() {
    uint8_t buf[BUFSIZ];
    for (;;) {
        ssize_t ret = read(m_pseudo_terminal.master_fd(), buf, sizeof(buf));
        if (ret < 0) {
            perror("terminal: read");
            exit(1);
        } else if (ret == 0) {
            break;
        }

        m_tty.scroll_to_bottom();
        m_tty.on_input({ buf, static_cast<size_t>(ret) });
    }
}
#endif /* __os_2__ */
