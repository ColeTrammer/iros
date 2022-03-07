#include <terminal/tty.h>
#include <tinput/terminal_renderer.h>
#include <tui/terminal_panel.h>

namespace TUI {
TerminalPanel::TerminalPanel() {}

Maybe<Point> TerminalPanel::cursor_position() {
    if (tty().cursor_hidden()) {
        return {};
    }
    return Point { tty().cursor_col(), tty().cursor_row() };
}

void TerminalPanel::render() {
    auto renderer = get_renderer();

    auto& rows = tty().rows();
    for (auto r = 0; r < tty().available_rows_in_display(); r++) {
        for (auto c = 0; c < tty().available_cols_in_display(); c++) {
            auto cell_rect = Rect { c, r, 1, 1 };
            if (r >= tty().row_count() || c >= tty().col_count()) {
                renderer.clear_rect(cell_rect);
                continue;
            }

            auto& row = rows[r];
            auto& cell = row[c];

            bool selected = in_selection(r, c);
            if (!cell.dirty && !selected) {
                continue;
            }

            cell.dirty = selected;

            auto fg = cell.fg;
            auto bg = cell.bg;

            if (selected) {
                swap(fg, bg);
            }

            renderer.render_text(cell_rect, { &cell.ch, 1 },
                                 { .foreground = fg, .background = bg, .bold = cell.bold, .invert = cell.inverted });
        }
    }
}

void TerminalPanel::invalidate_all_contents() {
    invalidate();
}

Rect TerminalPanel::available_cells() const {
    return sized_rect();
}

Point TerminalPanel::cell_position_of_mouse_coordinates(int mouse_x, int mouse_y) const {
    return { mouse_x, mouse_y };
}
}
