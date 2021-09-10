#include <app/context_menu.h>
#include <app/terminal_widget.h>
#include <app/window.h>
#include <clipboard/connection.h>
#include <ctype.h>
#include <errno.h>
#include <eventloop/event.h>
#include <graphics/renderer.h>
#include <unistd.h>

// #define TERMINAL_WIDGET_DEBUG

namespace App {
constexpr int cell_width = 8;
constexpr int cell_height = 16;

TerminalWidget::TerminalWidget(double opacity) : m_background_alpha(static_cast<uint8_t>(opacity * 255)) {}

void TerminalWidget::initialize() {
    auto context_menu = App::ContextMenu::create(parent_window()->shared_from_this(), parent_window()->shared_from_this());
    context_menu->add_menu_item("Copy", [this] {
        this->copy_selection();
    });
    context_menu->add_menu_item("Paste", [this] {
        this->paste_text();
    });
    set_context_menu(move(context_menu));

    Base::TerminalWidget::initialize();
    Widget::initialize();
}

Point TerminalWidget::cell_position_of_mouse_coordinates(int mouse_x, int mouse_y) const {
    return { mouse_x / cell_width, mouse_y / cell_height };
}

Rect TerminalWidget::available_cells() const {
    return { 0, 0, (sized_rect().width() - 10) / cell_width, (sized_rect().height() - 10) / cell_height };
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
    Rect right_rect = { 5 + cell_width * tty().col_count(), 0, 5, sized_rect().height() };
    Rect top_rect = { 5, 0, sized_rect().width() - 10, 5 };
    Rect bottom_rect = { 5, 5 + cell_height * tty().row_count(), sized_rect().width() - 10, 5 };

    Color default_bg = ColorValue::Black;
    default_bg.set_alpha(m_background_alpha);
    renderer.clear_rect(left_rect, default_bg);
    renderer.clear_rect(right_rect, default_bg);
    renderer.clear_rect(top_rect, default_bg);
    renderer.clear_rect(bottom_rect, default_bg);

    auto& rows = tty().rows();
    for (auto r = 0; r < tty().available_rows_in_display(); r++) {
        auto y = y_offset + r * cell_height;
        for (auto c = 0; c < tty().available_cols_in_display(); c++) {
            auto x = x_offset + c * cell_width;
            auto cell_rect = Rect { x, y, cell_width, cell_height };
            if (r >= tty().row_count() || c >= tty().col_count()) {
                renderer.clear_rect(cell_rect, default_bg);
                continue;
            }

            auto& row = rows[r];
            auto& cell = row[c];

            bool at_cursor = tty().should_display_cursor_at_position(r, c);
            bool selected = in_selection(r, c);
            if (!cell.dirty && !at_cursor && !selected) {
                continue;
            }

            cell.dirty = at_cursor || selected;

            auto fg = cell.fg.value_or(ColorValue::White);
            auto bg = cell.bg.value_or(default_bg);

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
}
