#include <app/context_menu.h>
#include <app/window.h>
#include <clipboard/connection.h>
#include <ctype.h>
#include <errno.h>
#include <eventloop/event.h>
#include <graphics/renderer.h>
#include <unistd.h>

#include "terminal_widget.h"

// #define TERMINAL_WIDGET_DEBUG

constexpr int cell_width = 8;
constexpr int cell_height = 16;

TerminalWidget::TerminalWidget(double opacity) : m_tty(m_pseudo_terminal), m_background_alpha(static_cast<uint8_t>(opacity * 255)) {}

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

        uint8_t buf[BUFSIZ];
        for (;;) {
            ssize_t ret = read(m_pseudo_terminal.master_fd(), buf, sizeof(buf));
            if (ret < 0) {
                if (errno != EAGAIN) {
                    perror("terminal: read");
                    exit(1);
                }
                break;
            } else if (ret == 0) {
                break;
            }

            m_tty.scroll_to_bottom();
            m_tty.on_input({ buf, static_cast<size_t>(ret) });
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

    auto renderer = get_renderer();
    auto x_offset = 5;
    auto y_offset = 5;

    Rect left_rect = { 0, 0, 5, sized_rect().height() };
    Rect right_rect = { 5 + cell_width * m_tty.col_count(), 0, 5, sized_rect().height() };
    Rect top_rect = { 5, 0, sized_rect().width() - 10, 5 };
    Rect bottom_rect = { 5, 5 + cell_height * m_tty.row_count(), sized_rect().width() - 10, 5 };

    Color default_bg = ColorValue::Black;
    default_bg.set_alpha(m_background_alpha);
    renderer.clear_rect(left_rect, default_bg);
    renderer.clear_rect(right_rect, default_bg);
    renderer.clear_rect(top_rect, default_bg);
    renderer.clear_rect(bottom_rect, default_bg);

    auto& rows = m_tty.rows();
    for (auto r = 0; r < m_tty.available_rows_in_display(); r++) {
        auto y = y_offset + r * cell_height;
        for (auto c = 0; c < m_tty.available_cols_in_display(); c++) {
            auto x = x_offset + c * cell_width;
            auto cell_rect = Rect { x, y, cell_width, cell_height };
            if (r >= m_tty.row_count() || c >= m_tty.col_count()) {
                renderer.clear_rect(cell_rect, default_bg);
                continue;
            }

            auto& row = rows[r];
            auto& cell = row[c];

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

            bg.set_alpha(m_background_alpha);
            renderer.clear_rect(cell_rect, bg);
            renderer.render_text(String(cell.ch), cell_rect, fg, TextAlign::Center, cell.bold ? Font::bold_font() : Font::default_font());
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

    int rows = (positioned_rect().height() - 10) / cell_height;
    int cols = (positioned_rect().width() - 10) / cell_width;
    m_tty.set_visible_size(rows, cols);
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

void TerminalWidget::on_key_event(const App::KeyEvent& event) {
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

void TerminalWidget::on_mouse_event(const App::MouseEvent& event) {
    auto cell_x = event.x() / cell_width;
    auto cell_y = event.y() / cell_height;

    int row_at_cursor = m_tty.scroll_relative_offset(cell_y);
    int col_at_cursor = cell_x;

    auto event_copy = App::MouseEvent(event.mouse_event_type(), event.buttons_down(), event.x(), event.y(), event.z(), event.button());
    event_copy.set_x(cell_x);
    event_copy.set_y(cell_y);
    if (m_pseudo_terminal.handle_mouse_event(event_copy)) {
        return;
    }

    if (event.mouse_scroll()) {
        if (event.z() < 0) {
            m_tty.scroll_up();
        } else if (event.z() > 0) {
            m_tty.scroll_down();
        }
        invalidate();
        return;
    }

    if (event.mouse_down_any() && event.button() == App::MouseButton::Left) {
        clear_selection();
        m_in_selection = true;
        switch (event.mouse_event_type()) {
            case App::MouseEventType::Down:
                m_selection_start_row = m_selection_end_row = row_at_cursor;
                m_selection_start_col = m_selection_end_col = col_at_cursor;
                invalidate();
                return;
            case App::MouseEventType::Double: {
                m_selection_start_row = m_selection_end_row = row_at_cursor;
                m_selection_start_col = m_selection_end_col = col_at_cursor;

                if (row_at_cursor < 0 || row_at_cursor >= m_tty.row_count()) {
                    m_in_selection = false;
                    return;
                }

                auto& row = m_tty.row_at_scroll_relative_offset(row_at_cursor);
                bool connect_spaces = isspace(row[col_at_cursor].ch);
                while (m_selection_start_col > 0 && isspace(row[m_selection_start_col - 1].ch) == connect_spaces) {
                    m_selection_start_col--;
                }
                while (m_selection_end_col < m_tty.col_count() - 1 && isspace(row[m_selection_end_col + 1].ch) == connect_spaces) {
                    m_selection_end_col++;
                }
                m_selection_end_col++;
                invalidate();
                return;
            }
            case App::MouseEventType::Triple:
                m_selection_start_row = m_selection_end_row = row_at_cursor;
                m_selection_start_col = 0;
                m_selection_end_col = m_tty.col_count();
                invalidate();
                return;
            default:
                break;
        }
    }

    if (event.mouse_up() && event.left_button()) {
        m_in_selection = false;
        return;
    }

    if (m_in_selection && (m_selection_end_row != row_at_cursor || m_selection_end_col != col_at_cursor)) {
        m_selection_end_row = row_at_cursor;
        m_selection_end_col = col_at_cursor;
        invalidate();
        return;
    }

    return Widget::on_mouse_event(event);
}
