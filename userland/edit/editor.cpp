#include <errno.h>
#include <liim/utilities.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "editor.h"
#include "panel.h"

#ifndef display_error
#define display_error(format, ...)                          \
    do {                                                    \
        fprintf(stderr, format __VA_OPT__(, ) __VA_ARGS__); \
        fprintf(stderr, ": %s\n", strerror(errno));         \
    } while (0)
#endif /* display_error */

LineSplitResult Line::split_at(int position) {
    StringView first = StringView(contents().string(), contents().string() + position - 1);
    StringView second = StringView(contents().string() + position);

    return { Line(first), Line(second) };
}

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

UniquePtr<Document> Document::create_empty(Panel& panel) {
    Vector<Line> lines;
    lines.add(Line(""));
    return make_unique<Document>(move(lines), "", panel);
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
        if (m_row_offset + m_panel.cursor_row() == m_lines.size() - 1) {
            return;
        }

        move_cursor_down();
        move_cursor_to_line_start();
        return;
    }

    if (cursor_col == m_panel.cols() - 1) {
        m_col_offset++;
        display();
        m_max_cursor_col = m_col_offset + m_panel.cols() - 1;
        return;
    }

    m_panel.set_cursor_col(cursor_col + 1);
    m_max_cursor_col = m_col_offset + cursor_col + 1;
}

void Document::move_cursor_down() {
    int cursor_row = m_panel.cursor_row();
    if (cursor_row + m_row_offset == m_lines.size() - 1) {
        move_cursor_to_line_end();
        return;
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
    } else if (m_col_offset + m_panel.cursor_col() < line.length()) {
        if (m_max_cursor_col > m_col_offset + m_panel.cursor_col()) {
            int new_cursor_col = LIIM::min(m_max_cursor_col, line.length());
            if (new_cursor_col >= m_panel.cols()) {
                m_col_offset = new_cursor_col - m_panel.cols() + 1;
                m_panel.set_cursor_col(m_panel.cols() - 1);
                display();
            } else {
                m_panel.set_cursor_col(new_cursor_col);
            }
        }
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
        m_max_cursor_col = m_col_offset;
        return;
    }

    m_panel.set_cursor_col(cursor_col - 1);
    m_max_cursor_col = m_col_offset + cursor_col - 1;
}

void Document::move_cursor_up() {
    int cursor_row = m_panel.cursor_row();
    if (cursor_row == 0 && m_row_offset == 0) {
        m_panel.set_cursor_col(0);
        m_max_cursor_col = 0;
        return;
    }

    if (cursor_row == 0) {
        m_row_offset--;
        display();
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
    m_max_cursor_col = 0;
}

void Document::move_cursor_to_line_end(UpdateMaxCursorCol should_update_max_cursor_col) {
    auto& line = line_at_cursor();

    if (should_update_max_cursor_col == UpdateMaxCursorCol::Yes) {
        m_max_cursor_col = line.length();
    }

    if (line.length() >= m_panel.cols()) {
        m_col_offset = line.length() - m_panel.cols() + 1;
        m_panel.set_cursor_col(m_panel.cols() - 1);
        display();
        return;
    }

    m_panel.set_cursor_col(line.length());

    if (m_col_offset != 0) {
        m_col_offset = 0;
        display();
    }
}

void Document::insert_char(char c) {
    auto& line = line_at_cursor();
    line.insert_char_at(m_panel.cursor_col() + m_col_offset, c);
    move_cursor_right();
    display();
}

void Document::merge_lines(int l1i, int l2i) {
    auto& l1 = m_lines[l1i];
    auto& l2 = m_lines[l2i];

    l1.combine_line(l2);
    m_lines.remove(l2i);
}

void Document::delete_char(DeleteCharMode mode) {
    auto& line = line_at_cursor();

    switch (mode) {
        case DeleteCharMode::Backspace: {
            if (line.empty()) {
                if (m_lines.size() == 0) {
                    return;
                }

                m_lines.remove(m_row_offset + m_panel.cursor_row());
                move_cursor_left();
                display();
                return;
            }

            int position = m_col_offset + m_panel.cursor_col();
            if (position == 0) {
                if (&line == &m_lines.first()) {
                    return;
                }

                int row_index = m_row_offset + m_panel.cursor_row();
                move_cursor_up();
                move_cursor_to_line_end();
                merge_lines(row_index - 1, row_index);
            } else {
                line.remove_char_at(position - 1);
                move_cursor_left();
            }

            display();
            break;
        }
        case DeleteCharMode::Delete:
            if (line.empty()) {
                if (m_lines.size() == 0) {
                    return;
                }

                m_lines.remove(m_row_offset + m_panel.cursor_row());
                display();
                return;
            }

            int position = m_col_offset + m_panel.cursor_col();
            if (position == line.length()) {
                if (&line == &m_lines.last()) {
                    return;
                }

                merge_lines(m_row_offset + m_panel.cursor_row(), m_row_offset + m_panel.cursor_row() + 1);
            } else {
                line.remove_char_at(position);
            }

            display();
            break;
    }
}

void Document::split_line_at_cursor() {
    auto& line = line_at_cursor();

    int row_index = m_row_offset + m_panel.cursor_row();
    auto result = line.split_at(m_col_offset + m_panel.cursor_col());

    line = move(result.first);
    m_lines.insert(move(result.second), row_index + 1);

    move_cursor_down();
    move_cursor_to_line_start();
    display();
}

void Document::save() {
    struct stat st;
    if (stat(m_name.string(), &st)) {
        m_panel.send_status_message(String::format("Error looking up file - `%s'", strerror(errno)));
        return;
    }

    if (access(m_name.string(), W_OK)) {
        m_panel.send_status_message(String::format("Permission to write file `%s' denied", m_name.string()));
        return;
    }

    auto temp_path_template = String::format("%sXXXXXX", m_name.string());
    char* temp_path = temp_path_template.string();
    int fd = mkstemp(temp_path);
    if (fd < 0) {
        m_panel.send_status_message(String::format("Failed to create a temp file - `%s'", strerror(errno)));
        return;
    }

    FILE* file = fdopen(fd, "w");
    if (!file) {
        m_panel.send_status_message(String::format("Failed to save - `%s'", strerror(errno)));
        return;
    }

    for (auto& line : m_lines) {
        fprintf(file, "%s\n", line.contents().string());
    }

    if (ferror(file)) {
        m_panel.send_status_message(String::format("Failed to write to disk - `%s'", strerror(errno)));
        fclose(file);
        return;
    }

    if (fchmod(fileno(file), st.st_mode)) {
        m_panel.send_status_message(String::format("Faild to sync file metadata - `%s'", strerror(errno)));
        fclose(file);
        return;
    }

    if (fclose(file)) {
        m_panel.send_status_message(String::format("Failed to sync to disk - `%s'", strerror(errno)));
        return;
    }

    if (rename(temp_path, m_name.string())) {
        m_panel.send_status_message(String::format("Failed to overwrite file - `%s'", strerror(errno)));
        return;
    }

    m_panel.send_status_message(String::format("Successfully saved file: `%s'", m_name.string()));
}

void Document::notify_key_pressed(KeyPress press) {
    if (press.modifiers & KeyPress::Modifier::Control) {
        switch (press.key) {
            case 'Q':
            case 'W':
                exit(0);
                break;
            case 'S':
                save();
                break;
            default:
                break;
        }

        return;
    }

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
        case KeyPress::Key::Backspace:
            delete_char(DeleteCharMode::Backspace);
            break;
        case KeyPress::Key::Delete:
            delete_char(DeleteCharMode::Delete);
            break;
        case KeyPress::Key::Enter:
            split_line_at_cursor();
            break;
        default:
            insert_char(press.key);
            break;
    }
}