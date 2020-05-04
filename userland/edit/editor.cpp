#include <errno.h>
#include <stdio.h>

#include "editor.h"
#include "panel.h"

#ifndef display_error
#define display_error(format, ...)                          \
    do {                                                    \
        fprintf(stderr, format __VA_OPT__(, ) __VA_ARGS__); \
        fprintf(stderr, ": %s\n", strerror(errno));         \
    } while (0)
#endif /* display_error */

UniquePtr<Document> Document::create_from_file(const String& path, Panel& panel) {
    FILE* file = fopen(path.string(), "r");
    if (!file) {
        display_error("edit: error reading file: `%s'", path.string());
        return nullptr;
    }

    Vector<Line> lines;
    char* line = nullptr;
    size_t line_max = 0;
    ssize_t line_length;
    while ((line_length = getline(&line, &line_max, file)) != -1) {
        char* trailing_newline = strchr(line, '\n');
        if (trailing_newline) {
            *trailing_newline = '\0';
        }

        lines.add(Line(String(line)));
    }

    UniquePtr<Document> ret;

    if (ferror(file)) {
        display_error("edit: error reading file: `%s'", path.string());
    } else {
        ret = make_unique<Document>(move(lines), path, panel);
    }

    if (fclose(file)) {
        display_error("edit: error closing file: `%s'", path.string());
    }

    return ret;
}

void Document::render_line(int line_number, int row_in_panel) const {
    auto& line = m_lines[line_number];
    for (int i = m_col_offset; i < line.length() && i - m_col_offset < m_panel.cols(); i++) {
        m_panel.set_text_at(row_in_panel, i - m_col_offset, line.contents()[i]);
    }
}

void Document::display() const {
    m_panel.clear();
    for (int line_num = m_row_offset; line_num < m_lines.size() && line_num - m_row_offset < m_panel.rows(); line_num++) {
        render_line(line_num, line_num - m_row_offset);
    }
    m_panel.flush();
}

Line& Document::line_at_cursor() {
    return m_lines[m_panel.cursor_row() + m_row_offset];
}

void Document::move_cursor_right() {
    int cursor_col = m_panel.cursor_col();
    auto& line = line_at_cursor();
    if (cursor_col + m_col_offset == line.length()) {
        move_cursor_down();
        move_cursor_to_line_start();
        return;
    }

    if (cursor_col == m_panel.cols() - 1) {
        m_col_offset++;
        display();
        return;
    }

    m_panel.set_cursor_col(cursor_col + 1);
}

void Document::move_cursor_down() {
    int cursor_row = m_panel.cursor_row();
    if (cursor_row == m_panel.rows() - 1) {
        if (m_row_offset == m_lines.size() - m_panel.rows()) {
            return;
        }
    }

    if (cursor_row == m_panel.rows() - 1) {
        m_row_offset++;
        display();
    } else {
        m_panel.set_cursor_row(cursor_row + 1);
    }

    clamp_cursor_to_line_end();
}

void Document::clamp_cursor_to_line_end() {
    auto& line = line_at_cursor();
    if (m_col_offset + m_panel.cursor_col() > line.length()) {
        move_cursor_to_line_end(UpdateMaxCursorCol::No);
    }
}

void Document::move_cursor_left() {
    int cursor_col = m_panel.cursor_col();
    if (cursor_col == 0) {
        if (m_col_offset == 0) {
            if (m_row_offset != 0 || m_panel.cursor_row() != 0) {
                move_cursor_up();
                move_cursor_to_line_end();
            }
            return;
        }

        m_col_offset--;
        display();
        return;
    }

    m_panel.set_cursor_col(cursor_col - 1);
}

void Document::move_cursor_up() {
    int cursor_row = m_panel.cursor_row();
    if (cursor_row == 0 && m_row_offset == 0) {
        m_panel.set_cursor_col(0);
        return;
    }

    if (cursor_row == 0) {
        display();
        m_row_offset--;
    } else {
        m_panel.set_cursor_row(cursor_row - 1);
    }

    clamp_cursor_to_line_end();
}

void Document::move_cursor_to_line_start() {
    m_panel.set_cursor_col(0);
    if (m_col_offset != 0) {
        m_col_offset = 0;
        display();
    }
}

void Document::move_cursor_to_line_end(UpdateMaxCursorCol should_update_max_cursor_col [[maybe_unused]]) {
    auto& line = line_at_cursor();
    if (line.length() >= m_panel.cols()) {
        m_col_offset = line.length() - m_panel.cols() + 1;
        m_panel.set_cursor_col(m_panel.cols() - 1);
        display();
        return;
    }

    m_panel.set_cursor_col(line.length());
}

void Document::notify_key_pressed(KeyPress press) {
    switch (press.key) {
        case KeyPress::Key::LeftArrow:
            move_cursor_left();
            break;
        case KeyPress::Key::RightArrow:
            move_cursor_right();
            break;
        case KeyPress::Key::DownArrow:
            move_cursor_down();
            break;
        case KeyPress::Key::UpArrow:
            move_cursor_up();
            break;
        case KeyPress::Key::Home:
            move_cursor_to_line_start();
            break;
        case KeyPress::Key::End:
            move_cursor_to_line_end();
            break;
        default:
            break;
    }
}