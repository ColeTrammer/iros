#include <app/event.h>
#include <app/window.h>
#include <graphics/pixel_buffer.h>
#include <graphics/renderer.h>

#include "app_panel.h"
#include "document.h"
#include "key_press.h"

AppPanel::AppPanel(int x, int y, int width, int height) {
    update_coordinates(x, y, width, height);
}

void AppPanel::update_coordinates(int x, int y, int width, int height) {
    set_rect({ x, y, width, height });
    m_rows = height / row_height();
    m_cols = width / col_width();
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
    if (m_cursor_row == row && m_cursor_col == col) {
        return;
    }

    m_cursor_row = row;
    m_cursor_col = col;
    if (document()) {
        document()->set_needs_display();
    }
}

void AppPanel::do_open_prompt() {}

void AppPanel::render_cell(Renderer& renderer, int x, int y, CellData& cell, bool at_cursor) {
    renderer.set_color(0);
    renderer.fill_rect(x, y, col_width(), row_height());

    renderer.set_color(Color(255, 255, 255));
    renderer.render_text(x, y, String(cell.c));

    if (at_cursor) {
        for (int yy = y; yy < y + row_height(); yy++) {
            renderer.pixels().put_pixel(x, yy, Color(255, 255, 255));
        }
    }
}

void AppPanel::render() {
    Renderer renderer(*window()->pixels());

    for (int r = 0; r < rows(); r++) {
        for (int c = 0; c < cols(); c++) {
            auto& cell = m_cells[index(r, c)];
            render_cell(renderer, rect().x() + c * col_width(), rect().y() + r * row_height(), cell,
                        r == cursor_row() && c == cursor_col());
        }
    }
}

void AppPanel::on_key_event(App::KeyEvent& event) {
    if (!document() || !event.key_down()) {
        return;
    }

    int modifiers = (event.shift_down() ? KeyPress::Modifier::Shift : 0) | (event.alt_down() ? KeyPress::Modifier::Alt : 0) |
                    (event.control_down() ? KeyPress::Modifier::Control : 0);
    int key = event.ascii();
    switch (event.key()) {
        case KEY_CURSOR_LEFT:
            key = KeyPress::Key::LeftArrow;
            break;
        case KEY_CURSOR_RIGHT:
            key = KeyPress::Key::RightArrow;
            break;
        case KEY_CURSOR_UP:
            key = KeyPress::Key::UpArrow;
            break;
        case KEY_CURSOR_DOWN:
            key = KeyPress::Key::DownArrow;
            break;
        case KEY_HOME:
            key = KeyPress::Key::Home;
            break;
        case KEY_END:
            key = KeyPress::Key::End;
            break;
        case KEY_BACKSPACE:
            key = KeyPress::Key::Backspace;
            break;
        case KEY_DELETE:
            key = KeyPress::Key::Delete;
            break;
        case KEY_ENTER:
            key = KeyPress::Key::Enter;
            break;
        case KEY_INSERT:
            key = KeyPress::Key::Insert;
            break;
        case KEY_ESC:
            key = KeyPress::Key::Escape;
            break;
        case KEY_PAGE_UP:
            key = KeyPress::Key::PageUp;
            break;
        case KEY_PAGE_DOWN:
            key = KeyPress::Key::PageDown;
            break;
        case KEY_F1:
            key = KeyPress::Key::F1;
            break;
        case KEY_F2:
            key = KeyPress::Key::F2;
            break;
        case KEY_F3:
            key = KeyPress::Key::F3;
            break;
        case KEY_F4:
            key = KeyPress::Key::F4;
            break;
        case KEY_F5:
            key = KeyPress::Key::F5;
            break;
        case KEY_F6:
            key = KeyPress::Key::F6;
            break;
        case KEY_F7:
            key = KeyPress::Key::F7;
            break;
        case KEY_F8:
            key = KeyPress::Key::F8;
            break;
        case KEY_F9:
            key = KeyPress::Key::F9;
            break;
        case KEY_F10:
            key = KeyPress::Key::F10;
            break;
        case KEY_F11:
            key = KeyPress::Key::F11;
            break;
        case KEY_F12:
            key = KeyPress::Key::F12;
            break;
        default:
            break;
    }

    if (!key) {
        return;
    }

    document()->notify_key_pressed({ modifiers, key });
}

void AppPanel::document_did_change() {
    if (document()) {
        notify_line_count_changed();
        document()->display();
    }
}
