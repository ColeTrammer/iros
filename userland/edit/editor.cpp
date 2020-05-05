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

constexpr int tab_width = 4;

LineSplitResult Line::split_at(int position) {
    StringView first = StringView(contents().string(), contents().string() + position - 1);
    StringView second = StringView(contents().string() + position);

    return { Line(first), Line(second) };
}

int Line::col_position_of_index(int index) const {
    int col = 0;
    for (int i = 0; i < index; i++) {
        char c = contents()[i];
        if (c == '\t') {
            col += tab_width - (col % tab_width);
        } else {
            col++;
        }
    }
    return col;
}

int Line::index_of_col_position(int position) const {
    int col = 0;
    int index;
    for (index = 0; index < length(); index++) {
        char c = contents()[index];
        int col_width = 1;
        if (c == '\t') {
            col_width = tab_width - (col % tab_width);
        }

        col += col_width;
        if (col > position) {
            break;
        }
    }
    return index;
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
        ret = make_unique<Document>(move(lines), path, panel, LineMode::Multiple);
    }

    if (fclose(file)) {
        display_error("edit: error closing file: `%s'", path.string());
    }

    return ret;
}

UniquePtr<Document> Document::create_empty(Panel& panel) {
    return make_unique<Document>(Vector<Line>(), "", panel, LineMode::Multiple);
}

UniquePtr<Document> Document::create_single_line(Panel& panel) {
    return make_unique<Document>(Vector<Line>(), "", panel, LineMode::Single);
}

Document::Document(Vector<Line> lines, String name, Panel& panel, LineMode mode)
    : m_lines(move(lines)), m_name(move(name)), m_panel(panel), m_line_mode(mode) {
    if (m_lines.empty()) {
        m_lines.add(Line(""));
    }
}

Document::~Document() {}

String Document::content_string() const {
    if (single_line_mode()) {
        return m_lines.first().contents();
    }

    String ret;
    for (auto& line : m_lines) {
        ret += line.contents();
        ret += "\n";
    }
    return ret;
}

void Document::render_line(int line_number, int row_in_panel) const {
    auto& line = m_lines[line_number];

    int col_position = 0;
    int line_index = line.index_of_col_position(m_col_offset);
    while (line_index < line.length() && col_position < m_panel.cols()) {
        char c = line.contents()[line_index];
        if (c == '\t') {
            int num_spaces = tab_width - ((col_position + m_col_offset) % tab_width);
            for (int i = 0; col_position < m_panel.cols() && i < num_spaces; i++) {
                m_panel.set_text_at(row_in_panel, col_position++, ' ');
            }
        } else {
            m_panel.set_text_at(row_in_panel, col_position++, c);
        }

        line_index++;
    }
}

void Document::display() const {
    m_panel.clear();
    for (int line_num = m_row_offset; line_num < m_lines.size() && line_num - m_row_offset < m_panel.rows(); line_num++) {
        render_line(line_num, line_num - m_row_offset);
    }
    m_panel.flush();
    m_needs_display = false;
}

Line& Document::line_at_cursor() {
    return m_lines[m_panel.cursor_row() + m_row_offset];
}

int Document::line_index_at_cursor() const {
    return line_at_cursor().index_of_col_position(cursor_col_position());
}

int Document::cursor_col_position() const {
    return m_panel.cursor_col() + m_col_offset;
}

void Document::move_cursor_right() {
    auto& line = line_at_cursor();
    int index_into_line = line_index_at_cursor();
    if (index_into_line == line.length()) {
        if (&line == &m_lines.last()) {
            return;
        }

        move_cursor_down();
        move_cursor_to_line_start();
        return;
    }

    int new_col_position = line.col_position_of_index(index_into_line + 1);
    int current_col_position = cursor_col_position();
    int cols_to_advance = new_col_position - current_col_position;

    m_max_cursor_col = new_col_position;

    int cursor_col = m_panel.cursor_col();
    if (cursor_col + cols_to_advance >= m_panel.cols()) {
        m_col_offset += m_panel.cols() - cols_to_advance - cursor_col + 1;
        m_panel.set_cursor_col(m_panel.cols() - 1);
        set_needs_display();
        return;
    }

    m_panel.set_cursor_col(m_panel.cursor_col() + cols_to_advance);
}

void Document::move_cursor_left() {
    auto& line = line_at_cursor();
    int index_into_line = line_index_at_cursor();
    if (index_into_line == 0) {
        if (&line == &m_lines.first()) {
            return;
        }

        move_cursor_up();
        move_cursor_to_line_end();
        return;
    }

    int new_col_position = line.col_position_of_index(index_into_line - 1);
    int current_col_position = cursor_col_position();
    int cols_to_recede = current_col_position - new_col_position;

    m_max_cursor_col = new_col_position;

    int cursor_col = m_panel.cursor_col();
    if (cursor_col - cols_to_recede < 0) {
        m_col_offset += cursor_col - cols_to_recede;
        m_panel.set_cursor_col(0);
        set_needs_display();
        return;
    }

    m_panel.set_cursor_col(m_panel.cursor_col() - cols_to_recede);
}

void Document::move_cursor_down() {
    int cursor_row = m_panel.cursor_row();
    if (cursor_row + m_row_offset == m_lines.size() - 1) {
        move_cursor_to_line_end();
        return;
    }

    if (cursor_row == m_panel.rows() - 1) {
        m_row_offset++;
        set_needs_display();
    } else {
        m_panel.set_cursor_row(cursor_row + 1);
    }

    clamp_cursor_to_line_end();
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
        set_needs_display();
    } else {
        m_panel.set_cursor_row(cursor_row - 1);
    }

    clamp_cursor_to_line_end();
}

void Document::clamp_cursor_to_line_end() {
    auto& line = line_at_cursor();
    int current_col = cursor_col_position();
    int max_col = line.col_position_of_index(line.length());
    if (current_col == max_col) {
        return;
    }

    if (current_col > max_col) {
        move_cursor_to_line_end(UpdateMaxCursorCol::No);
        return;
    }

    if (m_max_cursor_col > current_col) {
        int new_line_index = line.index_of_col_position(m_max_cursor_col);
        int new_cursor_col = line.col_position_of_index(new_line_index);
        if (new_cursor_col >= m_panel.cols()) {
            m_col_offset = new_cursor_col - m_panel.cols() - 1;
            m_panel.set_cursor_col(m_panel.cols() - 1);
            set_needs_display();
            return;
        }

        m_panel.set_cursor_col(new_cursor_col);
    }
}

void Document::move_cursor_to_line_start() {
    m_panel.set_cursor_col(0);
    if (m_col_offset != 0) {
        m_col_offset = 0;
        set_needs_display();
    }
    m_max_cursor_col = 0;
}

void Document::move_cursor_to_line_end(UpdateMaxCursorCol should_update_max_cursor_col) {
    auto& line = line_at_cursor();
    int new_col_position = line.col_position_of_index(line.length());

    if (should_update_max_cursor_col == UpdateMaxCursorCol::Yes) {
        m_max_cursor_col = new_col_position;
    }

    if (new_col_position >= m_panel.cols()) {
        m_col_offset = new_col_position - m_panel.cols() + 1;
        m_panel.set_cursor_col(m_panel.cols() - 1);
        set_needs_display();
        return;
    }

    m_panel.set_cursor_col(new_col_position);

    if (m_col_offset != 0) {
        m_col_offset = 0;
        set_needs_display();
    }
}

void Document::insert_char(char c) {
    auto& line = line_at_cursor();

    int col_position = cursor_col_position();
    int line_index = line.index_of_col_position(col_position);
    if (c == '\t' && convert_tabs_to_spaces()) {
        int num_spaces = tab_width - (col_position % tab_width);
        for (int i = 0; i < num_spaces; i++) {
            line.insert_char_at(line_index, ' ');
            move_cursor_right();
        }
    } else {
        line.insert_char_at(line_index, c);
        move_cursor_right();
    }
    set_needs_display();
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
                if (m_lines.size() == 1) {
                    return;
                }

                move_cursor_left();
                m_lines.remove(m_row_offset + m_panel.cursor_row());
                set_needs_display();
                return;
            }

            int index = line.index_of_col_position(cursor_col_position());
            if (index == 0) {
                if (&line == &m_lines.first()) {
                    return;
                }

                int row_index = m_row_offset + m_panel.cursor_row();
                move_cursor_up();
                move_cursor_to_line_end();
                merge_lines(row_index - 1, row_index);
            } else {
                move_cursor_left();
                line.remove_char_at(index - 1);
            }

            set_needs_display();
            break;
        }
        case DeleteCharMode::Delete:
            if (line.empty()) {
                if (m_lines.size() == 1) {
                    return;
                }

                m_lines.remove(m_row_offset + m_panel.cursor_row());
                set_needs_display();
                return;
            }

            int index = line.index_of_col_position(cursor_col_position());
            if (index == line.length()) {
                if (&line == &m_lines.last()) {
                    return;
                }

                merge_lines(m_row_offset + m_panel.cursor_row(), m_row_offset + m_panel.cursor_row() + 1);
            } else {
                line.remove_char_at(index);
            }

            set_needs_display();
            break;
    }
}

void Document::split_line_at_cursor() {
    auto& line = line_at_cursor();

    int row_index = m_row_offset + m_panel.cursor_row();
    auto result = line.split_at(line.index_of_col_position(cursor_col_position()));

    line = move(result.first);
    m_lines.insert(move(result.second), row_index + 1);

    move_cursor_down();
    move_cursor_to_line_start();
    set_needs_display();
}

void Document::save() {
    struct stat st;
    if (m_name.is_empty()) {
        String result = m_panel.prompt("Save as: ");
        if (access(result.string(), F_OK) == 0) {
            String ok = m_panel.prompt(String::format("Are you sure you want to overwrite file `%s'? ", result.string()));
            if (ok != "y" && ok != "yes") {
                return;
            }
        }

        m_name = move(result);
        st.st_mode = 0644;
    } else {
        if (stat(m_name.string(), &st)) {
            m_panel.send_status_message(String::format("Error looking up file - `%s'", strerror(errno)));
            return;
        }

        if (access(m_name.string(), W_OK)) {
            m_panel.send_status_message(String::format("Permission to write file `%s' denied", m_name.string()));
            return;
        }
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

    if (m_lines.size() == 1 && m_lines.first().empty()) {
        if (ftruncate(fileno(file), 0)) {
            m_panel.send_status_message(String::format("Failed to sync to disk - `%s'", strerror(errno)));
            fclose(file);
            return;
        }
    } else {
        for (auto& line : m_lines) {
            fprintf(file, "%s\n", line.contents().string());
        }
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
                if (!single_line_mode()) {
                    save();
                }
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
            if (!single_line_mode()) {
                split_line_at_cursor();
            }
            break;
        default:
            insert_char(press.key);
            break;
    }

    if (needs_display()) {
        display();
    }
}