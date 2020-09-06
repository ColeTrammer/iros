#include <app/context_menu.h>
#include <app/event.h>
#include <app/window.h>
#include <clipboard/connection.h>
#include <ctype.h>
#include <graphics/renderer.h>
#include <unistd.h>

#include "terminal_widget.h"

// #define TERMINAL_WIDGET_DEBUG

constexpr int cell_width = 8;
constexpr int cell_height = 16;

TerminalWidget::TerminalWidget() : m_tty(m_pseudo_terminal) {}

void TerminalWidget::initialize() {
    auto context_menu = App::ContextMenu::create(window()->shared_from_this(), window()->shared_from_this());
    context_menu->add_menu_item("Copy", [this] {
        this->copy_selection();
    });
    context_menu->add_menu_item("Paste", [this] {
        this->paste_text();
    });
    set_context_menu(move(context_menu));

    m_pseudo_terminal_wrapper = App::FdWrapper::create(shared_from_this(), m_pseudo_terminal.master_fd());
    m_pseudo_terminal_wrapper->set_selected_events(App::NotifyWhen::Readable);
    m_pseudo_terminal_wrapper->enable_notifications();
    m_pseudo_terminal_wrapper->on_readable = [this] {
#ifdef TERMINAL_WIDGET_DEBUG
        timespec start;
        clock_gettime(CLOCK_MONOTONIC, &start);
#endif /* TERMINAL_WIDGET_DEBUG */

        char buf[BUFSIZ];
        for (;;) {
            ssize_t ret = read(m_pseudo_terminal.master_fd(), buf, sizeof(buf));
            if (ret < 0) {
                perror("terminal: read");
                exit(1);
            } else if (ret == 0) {
                break;
            }

            m_tty.scroll_to_bottom();
            for (ssize_t i = 0; i < ret; i++) {
                m_tty.on_char(buf[i]);
            }
        }

#ifdef TERMINAL_WIDGET_DEBUG
        timespec end;
        clock_gettime(CLOCK_MONOTONIC, &end);

        long delta_seconds = end.tv_sec - start.tv_sec;
        long delta_nano_seconds = end.tv_nsec - start.tv_nsec;
        time_t delta_milli_seconds = delta_seconds * 1000 + delta_nano_seconds / 1000000;
        fprintf(stderr, "TerminalWidget: draining master fd took %lu ms\n", delta_milli_seconds);
#endif /* TERMINAL_WIDGET_DEBUG */

        invalidate();
    };
}

void TerminalWidget::render() {
#ifdef TERMINAL_WIDGET_DEBUG
    timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
#endif /* TERMINAL_WIDGET_DEBUG */

    Renderer renderer(*window()->pixels());
    auto x_offset = rect().x() + 5;
    auto y_offset = rect().y() + 5;

    auto& rows = m_tty.rows();
    for (auto r = 0; r < rows.size(); r++) {
        auto& row = rows[r];
        auto y = y_offset + r * cell_height;
        for (auto c = 0; c < row.size(); c++) {
            auto& cell = row[c];
            auto x = x_offset + c * cell_width;

            bool at_cursor = m_tty.should_display_cursor_at_position(r, c);
            bool selected = in_selection(r, c);
            if (!cell.dirty && !at_cursor && !selected) {
                continue;
            }

            cell.dirty = at_cursor || selected;

            auto fg = cell.fg;
            auto bg = cell.bg;

            if (at_cursor) {
                swap(fg, bg);
            }

            if (selected) {
                swap(fg, bg);
            }

            if (cell.inverted) {
                swap(fg, bg);
            }

            renderer.fill_rect({ x, y, cell_width, cell_height }, bg);
            renderer.render_text(x, y, String(cell.ch), fg, cell.bold ? Font::bold_font() : Font::default_font());
        }
    }

#ifdef TERMINAL_WIDGET_DEBUG
    timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);

    long delta_seconds = end.tv_sec - start.tv_sec;
    long delta_nano_seconds = end.tv_nsec - start.tv_nsec;
    time_t delta_milli_seconds = delta_seconds * 1000 + delta_nano_seconds / 1000000;
    fprintf(stderr, "TerminalWidget::render() took %lu ms\n", delta_milli_seconds);
#endif /* TERMINAL_WIDGET_DEBUG */
}

void TerminalWidget::on_resize() {
    m_selection_start_row = m_selection_start_col = m_selection_end_row = m_selection_end_col = -1;
    m_in_selection = false;

    int rows = (rect().height() - 10) / cell_height;
    int cols = (rect().width() - 10) / cell_width;
    m_tty.resize(rows, cols);
    m_pseudo_terminal.set_size(rows, cols);
}

void TerminalWidget::copy_selection() {
    auto text = selection_text();
    if (!text.is_empty()) {
        Clipboard::Connection::the().set_clipboard_contents_to_text(text);
    }
}

void TerminalWidget::paste_text() {
    auto maybe_text = Clipboard::Connection::the().get_clipboard_contents_as_text();
    if (!maybe_text.has_value()) {
        return;
    }
    m_pseudo_terminal.send_clipboard_contents(maybe_text.value());
}

void TerminalWidget::on_key_event(App::KeyEvent& event) {
    if (event.key_down() && event.control_down() && event.shift_down() && event.key() == KEY_C) {
        copy_selection();
        return;
    }

    if (event.ascii()) {
        clear_selection();
    }

    if (event.key_down() && event.control_down() && event.shift_down() && event.key() == KEY_V) {
        paste_text();
        return;
    }

    m_pseudo_terminal.handle_key_event(event.key(), event.flags(), event.ascii());
}

void TerminalWidget::clear_selection() {
    m_in_selection = false;
    if (m_selection_start_row == -1 || m_selection_start_col == -1 || m_selection_end_row == -1 || m_selection_end_col == -1) {
        return;
    }

    m_selection_start_row = m_selection_start_col = m_selection_end_row = m_selection_end_col = -1;
    invalidate();
}

bool TerminalWidget::in_selection(int relative_row, int col) const {
    int start_row = m_selection_start_row;
    int start_col = m_selection_start_col;
    int end_row = m_selection_end_row;
    int end_col = m_selection_end_col;

    if (start_row == -1 || start_col == -1 || end_row == -1 || end_col == -1) {
        return false;
    }

    if (start_row == end_row && start_col == end_col) {
        return false;
    }

    if ((end_row == start_row && end_col < start_col) || end_row < start_row) {
        swap(start_row, end_row);
        swap(start_col, end_col);
    }

    auto row = m_tty.scroll_relative_offset(relative_row);
    if (row > start_row && row < end_row) {
        return true;
    }

    if (row == start_row) {
        return col >= start_col && (row == end_row ? col < end_col : true);
    }

    return row == end_row && col < end_col;
}

String TerminalWidget::selection_text() const {
    int start_row = m_selection_start_row;
    int start_col = m_selection_start_col;
    int end_row = m_selection_end_row;
    int end_col = m_selection_end_col;

    if (start_row == -1 || start_col == -1 || end_row == -1 || end_col == -1) {
        return "";
    }

    if (start_row == end_row && start_col == end_col) {
        return "";
    }

    if ((end_row == start_row && end_col < start_col) || end_row < start_row) {
        swap(start_row, end_row);
        swap(start_col, end_col);
    }

    String text;
    for (auto r = start_row; r <= end_row; r++) {
        String row_text;
        auto iter_start_col = r == start_row ? start_col : 0;
        auto iter_end_col = r == end_row ? end_col : m_tty.col_count();

        for (auto c = iter_start_col; c < iter_end_col; c++) {
            row_text += String(m_tty.row_at_scroll_relative_offset(r)[c].ch);
        }

        while (!row_text.is_empty() && isspace(row_text[row_text.size() - 1])) {
            row_text.remove_index(row_text.size() - 1);
        }

        text += row_text;
        if (iter_end_col == m_tty.col_count()) {
            text += "\n";
        }
    }
    return text;
}

void TerminalWidget::on_mouse_event(App::MouseEvent& event) {
    if (event.scroll() == SCROLL_DOWN) {
        m_tty.scroll_down();
        invalidate();
        return;
    }

    if (event.scroll() == SCROLL_UP) {
        m_tty.scroll_up();
        invalidate();
        return;
    }

    int row_at_cursor = m_tty.scroll_relative_offset(event.y() / cell_height);
    int col_at_cursor = event.x() / cell_width;
    if (m_pseudo_terminal.handle_mouse_event(event.left(), event.right(), row_at_cursor, col_at_cursor, event.scroll())) {
        return;
    }

    if (event.left() == MOUSE_DOWN) {
        clear_selection();
        m_in_selection = true;
        m_selection_start_row = row_at_cursor;
        m_selection_start_col = col_at_cursor;
        return;
    }

    if (event.left() == MOUSE_UP) {
        m_in_selection = false;
        return;
    }

    if (m_in_selection && (m_selection_end_row != row_at_cursor || m_selection_end_col != col_at_cursor)) {
        m_selection_end_row = row_at_cursor;
        m_selection_end_col = col_at_cursor;
        invalidate();
        return;
    }

    Widget::on_mouse_event(event);
}
