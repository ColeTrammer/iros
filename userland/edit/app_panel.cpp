#include <app/window.h>
#include <graphics/renderer.h>

#include "app_panel.h"
#include "document.h"

AppPanel::AppPanel(int x, int y, int width, int height) {
    update_coordinates(x, y, width, height);
}

void AppPanel::update_coordinates(int x, int y, int width, int height) {
    set_rect({ x, y, width, height });
    m_rows = width / col_width();
    m_cols = height / row_height();
    clear();
    if (document()) {
        document()->notify_panel_size_changed();
    }
}

AppPanel::~AppPanel() {}

void AppPanel::clear() {
    m_cells.resize(m_rows * m_cols);
    for (auto& cell : m_cells) {
        cell.c = ' ';
        cell.metadata = CharacterMetadata();
    }
}

void AppPanel::set_text_at(int row, int col, char c, CharacterMetadata metadata) {
    m_cells[index(row, col)] = { c, metadata };
}

void AppPanel::flush() {
    window()->draw();
}

void AppPanel::enter() {
    window()->set_focused_widget(*this);
};

void AppPanel::send_status_message(String) {}

String AppPanel::prompt(const String&) {
    return "";
}

void AppPanel::enter_search(String) {}

void AppPanel::notify_line_count_changed() {}

void AppPanel::set_clipboard_contents(String, bool) {}

String AppPanel::clipboard_contents(bool&) const {
    return "";
}

void AppPanel::set_cursor(int row, int col) {
    m_cursor_row = row;
    m_cursor_col = col;
}

void AppPanel::do_open_prompt() {}

void AppPanel::render_cell(Renderer& renderer, int x, int y, CellData& cell) {
    renderer.set_color(0);
    renderer.fill_rect(x, y, col_width(), row_height());

    renderer.set_color(Color(255, 255, 255));
    renderer.render_text(x, y, String(cell.c));
}

void AppPanel::render() {
    Renderer renderer(*window()->pixels());

    for (int r = 0; r < rows(); r++) {
        for (int c = 0; c < cols(); c++) {
            auto& cell = m_cells[index(r, c)];
            render_cell(renderer, rect().x() + c * col_width(), rect().y() + r * row_height(), cell);
        }
    }
}

void AppPanel::document_did_change() {
    if (document()) {
        notify_line_count_changed();
        document()->display();
    }
}
