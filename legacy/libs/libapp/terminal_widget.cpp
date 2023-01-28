#include <app/terminal_widget.h>
#include <app/terminal_widget_bridge.h>
#include <app/widget_bridge.h>
#include <app/window.h>
#include <clipboard/connection.h>
#include <ctype.h>
#include <errno.h>
#include <eventloop/event.h>
#include <eventloop/event_loop.h>
#include <eventloop/object.h>
#include <eventloop/selectable.h>
#include <graphics/renderer.h>
#include <unistd.h>

// #define TERMINAL_WIDGET_DEBUG

namespace App {
TerminalWidget::TerminalWidget(SharedPtr<WidgetBridge> widget_bridge, SharedPtr<TerminalWidgetBridge> terminal_bridge)
    : Widget(move(widget_bridge)), m_tty(m_pseudo_terminal), m_bridge(move(terminal_bridge)) {}

TerminalWidget::~TerminalWidget() {}

void TerminalWidget::initialize() {
    auto key_bindings = App::KeyBindings {};
    key_bindings.add({ App::Key::C, App::KeyModifier::Control | App::KeyModifier::Shift }, [this] {
        copy_selection();
    });
    key_bindings.add({ App::Key::V, App::KeyModifier::Control | App::KeyModifier::Shift }, [this] {
        paste_text();
    });
    set_key_bindings(move(key_bindings));

    set_accepts_focus(true);

    m_pseudo_terminal_wrapper = add<App::FdWrapper>(m_pseudo_terminal.master_fd());
    m_pseudo_terminal_wrapper->set_selected_events(App::NotifyWhen::Readable);
    m_pseudo_terminal_wrapper->enable_notifications();
    listen<App::ReadableEvent>(*m_pseudo_terminal_wrapper, [this](auto&) {
#ifdef TERMINAL_WIDGET_DEBUG
        timespec start;
        clock_gettime(CLOCK_MONOTONIC, &start);
#endif /* TERMINAL_WIDGET_DEBUG */

        uint8_t buf[BUFSIZ];
        for (;;) {
            ssize_t ret = read(m_pseudo_terminal.master_fd(), buf, sizeof(buf));
            if (ret < 0) {
                if (errno != EAGAIN) {
                    EventLoop::queue_event(weak_from_this(), make_unique<App::TerminalHangupEvent>());
                    return;
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

        bridge().invalidate_all_contents();
    });

    on<App::ResizeEvent>([this](auto&) {
        m_selection_start_row = m_selection_start_col = m_selection_end_row = m_selection_end_col = -1;
        m_in_selection = false;

        auto size = available_cells();
        int rows = size.height();
        int cols = size.width();
        m_tty.set_visible_size(rows, cols);
        m_pseudo_terminal.set_size(rows, cols);
    });

    on<App::ShowEvent>([this](auto&) {
        m_tty.invalidate_all();
        bridge().invalidate_all_contents();
    });

    on<App::KeyDownEvent>([this](const App::KeyDownEvent& event) {
        if (Widget::key_bindings().handle_key_event(event)) {
            return true;
        }

        m_pseudo_terminal.handle_key_event(event);
        return true;
    });

    on<App::TextEvent>([this](const App::TextEvent& event) {
        m_pseudo_terminal.handle_text_event(event);
        return true;
    });

    on<App::MouseDownEvent, App::MouseMoveEvent, App::MouseUpEvent, App::MouseScrollEvent>([this](const auto& event) {
        using SpecificMouseEvent = LIIM::decay_t<decltype(event)>;

        auto cell = cell_position_of_mouse_coordinates(event.x(), event.y());
        auto cell_x = cell.x();
        auto cell_y = cell.y();

        int row_at_cursor = m_tty.scroll_relative_offset(cell_y);
        int col_at_cursor = cell_x;

        auto event_copy = SpecificMouseEvent { event };
        event_copy.set_x(cell_x);
        event_copy.set_y(cell_y);
        if (m_pseudo_terminal.handle_mouse_event(event_copy)) {
            return true;
        }

        if (event.mouse_scroll()) {
            if (event.z() < 0) {
                m_tty.scroll_up();
            } else if (event.z() > 0) {
                m_tty.scroll_down();
            }
            bridge().invalidate_all_contents();
            return true;
        }

        if (event.mouse_down() && event.left_button()) {
            clear_selection();
            m_in_selection = true;

            switch (event.cyclic_count(3)) {
                case 1: {
                    m_selection_start_row = m_selection_end_row = row_at_cursor;
                    m_selection_start_col = m_selection_end_col = col_at_cursor;
                    bridge().invalidate_all_contents();
                    break;
                }
                case 2: {
                    m_selection_start_row = m_selection_end_row = row_at_cursor;
                    m_selection_start_col = m_selection_end_col = col_at_cursor;

                    if (row_at_cursor < 0 || row_at_cursor >= m_tty.row_count()) {
                        m_in_selection = false;
                        return true;
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
                    bridge().invalidate_all_contents();
                    break;
                }
                case 3: {
                    m_selection_start_row = m_selection_end_row = row_at_cursor;
                    m_selection_start_col = 0;
                    m_selection_end_col = m_tty.col_count();
                    bridge().invalidate_all_contents();
                    break;
                }
            }
            return true;
        }

        if (event.mouse_up() && event.left_button()) {
            m_in_selection = false;
            return true;
        }

        if (m_in_selection && (m_selection_end_row != row_at_cursor || m_selection_end_col != col_at_cursor)) {
            m_selection_end_row = row_at_cursor;
            m_selection_end_col = col_at_cursor;
            bridge().invalidate_all_contents();
            return true;
        }

        return false;
    });

    Widget::initialize();
}

void TerminalWidget::copy_selection() {
    auto text = selection_text();
    if (!text.empty()) {
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

void TerminalWidget::clear_selection() {
    m_in_selection = false;
    if (m_selection_start_row == -1 || m_selection_start_col == -1 || m_selection_end_row == -1 || m_selection_end_col == -1) {
        return;
    }

    m_selection_start_row = m_selection_start_col = m_selection_end_row = m_selection_end_col = -1;
    bridge().invalidate_all_contents();
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

        while (!row_text.empty() && isspace(row_text[row_text.size() - 1])) {
            row_text.remove_index(row_text.size() - 1);
        }

        text += row_text;
        if (iter_end_col == m_tty.col_count()) {
            text += "\n";
        }
    }
    return text;
}
}
