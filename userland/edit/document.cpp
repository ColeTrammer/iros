#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "command.h"
#include "document.h"
#include "key_press.h"
#include "mouse_event.h"
#include "panel.h"

static inline int isword(int c) {
    return isalnum(c) || c == '_';
}

UniquePtr<Document> Document::create_from_stdin(const String& path, Panel& panel) {
    Vector<Line> lines;
    char* line = nullptr;
    size_t line_max = 0;
    ssize_t line_length;
    while ((line_length = getline(&line, &line_max, stdin)) != -1) {
        char* trailing_newline = strchr(line, '\n');
        if (trailing_newline) {
            *trailing_newline = '\0';
        }

        lines.add(Line(String(line)));
    }

    UniquePtr<Document> ret;

    if (ferror(stdin)) {
        panel.send_status_message(String::format("error reading stdin: `%s'", strerror(errno)));
        ret = Document::create_empty(panel);
        ret->set_name(path);
    } else {
        ret = make_unique<Document>(move(lines), path, panel, LineMode::Multiple);
    }

    assert(freopen("/dev/tty", "r+", stdin));
    return ret;
}

UniquePtr<Document> Document::create_from_file(const String& path, Panel& panel) {
    FILE* file = fopen(path.string(), "r");
    if (!file) {
        if (errno == ENOENT) {
            panel.send_status_message(String::format("new file: `%s'", path.string()));
            return make_unique<Document>(Vector<Line>(), path, panel, LineMode::Multiple);
        }
        panel.send_status_message(String::format("error accessing file: `%s': `%s'", path.string(), strerror(errno)));
        return Document::create_empty(panel);
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
        panel.send_status_message(String::format("error reading file: `%s'", path.string()));
        ret = Document::create_empty(panel);
    } else {
        ret = make_unique<Document>(move(lines), path, panel, LineMode::Multiple);
    }

    if (fclose(file)) {
        panel.send_status_message(String::format("error closing file: `%s'", path.string()));
    }

    return ret;
}

UniquePtr<Document> Document::create_empty(Panel& panel) {
    return make_unique<Document>(Vector<Line>(), "", panel, LineMode::Multiple);
}

UniquePtr<Document> Document::create_single_line(Panel& panel, String text) {
    Vector<Line> lines;
    lines.add(Line(move(text)));
    auto ret = make_unique<Document>(move(lines), "", panel, LineMode::Single);
    ret->set_show_line_numbers(false);
    ret->move_cursor_to_line_end();
    return ret;
}

Document::Document(Vector<Line> lines, String name, Panel& panel, LineMode mode)
    : m_lines(move(lines)), m_name(move(name)), m_panel(panel), m_line_mode(mode) {
    if (m_lines.empty()) {
        m_lines.add(Line(""));
    }
    guess_type_from_name();
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

void Document::display_if_needed() const {
    if (needs_display()) {
        display();
    }
}

void Document::display() const {
    for (int line_num = m_row_offset; line_num < m_lines.size() && line_num - m_row_offset < m_panel.rows(); line_num++) {
        m_lines[line_num].render(const_cast<Document&>(*this).panel(), m_col_offset, line_num - m_row_offset);
    }
    m_panel.flush();
    m_needs_display = false;
}

Line& Document::line_at_cursor() {
    return m_lines[m_panel.cursor_row() + m_row_offset];
}

char Document::char_at_cursor() const {
    auto& line = line_at_cursor();
    return line.char_at(line.index_of_col_position(cursor_col_position()));
}

int Document::index_of_line_at_cursor() const {
    return cursor_row_position();
}

int Document::index_of_line_at_position(int position) const {
    return position + m_row_offset;
}

int Document::line_index_at_cursor() const {
    return line_at_cursor().index_of_col_position(cursor_col_position());
}

int Document::index_into_line(int index_of_line, int position) const {
    return line_at_index(index_of_line).index_of_col_position(position + m_col_offset);
}

int Document::cursor_col_position() const {
    return m_panel.cursor_col() + m_col_offset;
}

int Document::cursor_row_position() const {
    return m_panel.cursor_row() + m_row_offset;
}

bool Document::cursor_at_document_start() const {
    return cursor_row_position() == 0 && cursor_col_position() == 0;
}

bool Document::cursor_at_document_end() const {
    return cursor_row_position() == num_lines() - 1 && cursor_col_position() == m_lines.last().length();
}

void Document::update_selection_state_for_mode(MovementMode mode) {
    if (mode == MovementMode::Move) {
        clear_selection();
        return;
    }

    if (m_selection.empty()) {
        m_selection.begin(cursor_row_position(), line_index_at_cursor());
    }
}

void Document::move_cursor_right(MovementMode mode) {
    scroll_cursor_into_view();

    auto& line = line_at_cursor();
    int index_into_line = line_index_at_cursor();
    if (index_into_line == line.length()) {
        if (&line == &m_lines.last()) {
            return;
        }

        move_cursor_down(mode);
        move_cursor_to_line_start(mode);
        return;
    }

    int new_col_position = line.col_position_of_index(index_into_line + 1);
    int current_col_position = cursor_col_position();
    int cols_to_advance = new_col_position - current_col_position;

    m_max_cursor_col = new_col_position;

    if (!m_selection.empty() && mode == MovementMode::Move) {
        int line_end = m_selection.lower_line();
        int index = m_selection.lower_index();
        clear_selection();
        move_cursor_to(line_end, index);
        return;
    }

    if (mode == MovementMode::Select) {
        if (m_selection.empty()) {
            m_selection.begin(cursor_row_position(), line_index_at_cursor());
        }
        m_selection.set_end_index(index_into_line + 1);
        line.metadata_at(index_into_line).invert_selected();
        set_needs_display();
    }

    int cursor_col = m_panel.cursor_col();
    if (cursor_col + cols_to_advance >= m_panel.cols()) {
        m_col_offset += cursor_col + cols_to_advance - m_panel.cols() + 1;
        m_panel.set_cursor_col(m_panel.cols() - 1);
        set_needs_display();
        return;
    }

    m_panel.set_cursor_col(m_panel.cursor_col() + cols_to_advance);
}

void Document::move_cursor_right_by_word(MovementMode mode) {
    move_cursor_right(mode);

    auto& line = line_at_cursor();
    while (line.index_of_col_position(cursor_col_position()) < line.length() && !isword(char_at_cursor())) {
        move_cursor_right(mode);
    }

    while (line.index_of_col_position(cursor_col_position()) < line.length() && isword(char_at_cursor())) {
        move_cursor_right(mode);
    }
}

void Document::move_cursor_left_by_word(MovementMode mode) {
    move_cursor_left(mode);

    while (cursor_col_position() > 0 && !isword(char_at_cursor())) {
        move_cursor_left(mode);
    }

    bool found_word = false;
    while (cursor_col_position() > 0 && isword(char_at_cursor())) {
        move_cursor_left(mode);
        found_word = true;
    }

    if (found_word && !isword(char_at_cursor())) {
        move_cursor_right(mode);
    }
}

void Document::move_cursor_left(MovementMode mode) {
    scroll_cursor_into_view();

    auto& line = line_at_cursor();
    int index_into_line = line_index_at_cursor();
    if (index_into_line == 0) {
        if (&line == &m_lines.first()) {
            return;
        }

        move_cursor_up(mode);
        move_cursor_to_line_end(mode);
        return;
    }

    int new_col_position = line.col_position_of_index(index_into_line - 1);
    int current_col_position = cursor_col_position();
    int cols_to_recede = current_col_position - new_col_position;

    m_max_cursor_col = new_col_position;

    if (!m_selection.empty() && mode == MovementMode::Move) {
        int line_start = m_selection.upper_line();
        int index = m_selection.upper_index();
        clear_selection();
        move_cursor_to(line_start, index);
        return;
    }

    if (mode == MovementMode::Select) {
        if (m_selection.empty()) {
            m_selection.begin(cursor_row_position(), line_index_at_cursor());
        }
        m_selection.set_end_index(index_into_line - 1);
        line.metadata_at(index_into_line - 1).invert_selected();
        set_needs_display();
    }

    int cursor_col = m_panel.cursor_col();
    if (cursor_col - cols_to_recede < 0) {
        m_col_offset += cursor_col - cols_to_recede;
        m_panel.set_cursor_col(0);
        set_needs_display();
        return;
    }

    m_panel.set_cursor_col(m_panel.cursor_col() - cols_to_recede);
}

void Document::move_cursor_down(MovementMode mode) {
    scroll_cursor_into_view();

    int cursor_row = m_panel.cursor_row();
    if (cursor_row + m_row_offset == m_lines.size() - 1) {
        move_cursor_to_line_end(mode);
        return;
    }

    auto& prev_line = line_at_cursor();
    int prev_line_index = line_index_at_cursor();
    update_selection_state_for_mode(mode);

    if (cursor_row == m_panel.rows() - 1) {
        m_row_offset++;
        set_needs_display();
    } else {
        m_panel.set_cursor_row(cursor_row + 1);
    }

    int new_line_index = clamp_cursor_to_line_end();
    if (mode == MovementMode::Select) {
        prev_line.toggle_select_after(prev_line_index);
        auto& new_line = line_at_cursor();
        new_line.toggle_select_before(new_line_index);
        m_selection.set_end_line(cursor_row_position());
        m_selection.set_end_index(new_line_index);
        set_needs_display();
    }
}

void Document::move_cursor_up(MovementMode mode) {
    scroll_cursor_into_view();

    int cursor_row = m_panel.cursor_row();
    if (cursor_row == 0 && m_row_offset == 0) {
        move_cursor_to_line_start(mode);
        return;
    }

    auto& prev_line = line_at_cursor();
    int prev_line_index = line_index_at_cursor();
    update_selection_state_for_mode(mode);

    if (cursor_row == 0) {
        m_row_offset--;
        set_needs_display();
    } else {
        m_panel.set_cursor_row(cursor_row - 1);
    }

    int new_line_index = clamp_cursor_to_line_end();
    if (mode == MovementMode::Select) {
        prev_line.toggle_select_before(prev_line_index);
        auto& new_line = line_at_cursor();
        new_line.toggle_select_after(new_line_index);
        m_selection.set_end_line(cursor_row_position());
        m_selection.set_end_index(new_line_index);
        set_needs_display();
    }
}

int Document::clamp_cursor_to_line_end() {
    auto& line = line_at_cursor();
    int current_col = cursor_col_position();
    int max_col = line.col_position_of_index(line.length());
    if (current_col == max_col) {
        return line.length();
    }

    if (current_col > max_col) {
        if (max_col >= m_panel.cols()) {
            m_col_offset = max_col - m_panel.cols() + 1;
            m_panel.set_cursor_col(m_panel.cols() - 1);
            set_needs_display();
            return line.length();
        }

        m_panel.set_cursor_col(max_col);

        if (m_col_offset != 0) {
            m_col_offset = 0;
            set_needs_display();
        }
        return line.length();
    }

    if (m_max_cursor_col > current_col) {
        int new_line_index = line.index_of_col_position(m_max_cursor_col);
        int new_cursor_col = line.col_position_of_index(new_line_index);
        if (new_cursor_col >= m_panel.cols()) {
            m_col_offset = new_cursor_col - m_panel.cols() + 1;
            m_panel.set_cursor_col(m_panel.cols() - 1);
            set_needs_display();
            return new_line_index;
        }

        m_panel.set_cursor_col(new_cursor_col);
        return new_line_index;
    }

    return line_index_at_cursor();
}

void Document::move_cursor_to_line_start(MovementMode mode) {
    scroll_cursor_into_view();

    update_selection_state_for_mode(mode);
    if (mode == MovementMode::Select) {
        auto& line = line_at_cursor();
        int line_index = line_index_at_cursor();
        line.toggle_select_before(line_index);
        m_selection.set_end_index(0);
        set_needs_display();
    }

    m_panel.set_cursor_col(0);
    if (m_col_offset != 0) {
        m_col_offset = 0;
        set_needs_display();
    }
    m_max_cursor_col = 0;
}

void Document::move_cursor_to_line_end(MovementMode mode) {
    scroll_cursor_into_view();

    auto& line = line_at_cursor();
    int new_col_position = line.col_position_of_index(line.length());

    m_max_cursor_col = new_col_position;

    update_selection_state_for_mode(mode);
    if (mode == MovementMode::Select) {
        int line_index = line_index_at_cursor();
        line.toggle_select_after(line_index);
        m_selection.set_end_index(line.length());
        set_needs_display();
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

void Document::move_cursor_to_document_start(MovementMode mode) {
    move_cursor_to(0, 0, mode);
}

void Document::move_cursor_to_document_end(MovementMode mode) {
    int last_line_index = m_lines.size() - 1;
    auto& last_line = m_lines.last();
    move_cursor_to(last_line_index, last_line.length(), mode);
}

void Document::scroll(int z) {
    if (z < 0) {
        scroll_up(-z);
    } else if (z > 0) {
        scroll_down(z);
    }
}

void Document::scroll_up(int times) {
    for (int i = 0; i < times; i++) {
        if (m_row_offset > 0) {
            m_row_offset--;
            m_panel.set_cursor_row(m_panel.cursor_row() + 1);
            set_needs_display();
        }
    }
}

void Document::scroll_down(int times) {
    for (int i = 0; i < times; i++) {
        if (m_row_offset + m_panel.rows() < num_lines()) {
            m_row_offset++;
            m_panel.set_cursor_row(m_panel.cursor_row() - 1);
            set_needs_display();
        }
    }
}

void Document::scroll_left(int times) {
    for (int i = 0; i < times; i++) {
        if (m_col_offset > 0) {
            m_col_offset--;
            m_panel.set_cursor_col(m_panel.cursor_col() + 1);
            set_needs_display();
        }
    }
}

void Document::scroll_right(int times) {
    for (int i = 0; i < times; i++) {
        m_col_offset++;
        m_panel.set_cursor_col(m_panel.cursor_col() - 1);
        set_needs_display();
    }
}

void Document::scroll_cursor_into_view() {
    if (m_panel.cursor_row() < 0) {
        scroll_up(-m_panel.cursor_row());
    } else if (m_panel.cursor_row() >= m_panel.rows()) {
        scroll_down(m_panel.cursor_row() - m_panel.rows() + 1);
    }

    if (m_panel.cursor_col() < 0) {
        scroll_right(-m_panel.cursor_col());
    } else if (m_panel.cursor_col() >= m_panel.cols()) {
        scroll_left(m_panel.cursor_col() - m_panel.cols() + 1);
    }
}

void Document::move_cursor_page_up(MovementMode mode) {
    int rows_to_move = m_panel.rows() - 1;

    for (int i = 0; !cursor_at_document_start() && i < rows_to_move; i++) {
        move_cursor_up(mode);
    }
}

void Document::move_cursor_page_down(MovementMode mode) {
    int rows_to_move = m_panel.rows() - 1;

    for (int i = 0; !cursor_at_document_end() <= num_lines() && i < rows_to_move; i++) {
        move_cursor_down(mode);
    }
}

void Document::merge_lines(int l1i, int l2i) {
    auto& l1 = m_lines[l1i];
    auto& l2 = m_lines[l2i];

    l1.combine_line(l2);
    remove_line(l2i);
    set_needs_display();
}

void Document::insert_char(char c) {
    push_command<InsertCommand>(String(c));
}

void Document::delete_char(DeleteCharMode mode) {
    push_command<DeleteCommand>(mode);
}

void Document::delete_word(DeleteCharMode mode) {
    if (!m_selection.empty()) {
        delete_char(mode);
        return;
    }

    int line_index = line_index_at_cursor();
    if ((mode == DeleteCharMode::Backspace && line_index == 0) ||
        (mode == DeleteCharMode::Delete && line_index == line_at_cursor().length())) {
        delete_char(mode);
        return;
    }

    if (mode == DeleteCharMode::Backspace) {
        move_cursor_left_by_word(MovementMode::Select);
    } else {
        move_cursor_right_by_word(MovementMode::Select);
    }

    swap_selection_start_and_cursor();
    push_command<DeleteCommand>(mode, true);
}

void Document::swap_selection_start_and_cursor() {
    int dest_line = m_selection.start_line();
    int dest_index = m_selection.start_index();
    int old_end_line = m_selection.end_line();
    int old_end_index = m_selection.end_index();

    move_cursor_to(dest_line, dest_index);

    m_selection.set_start_line(dest_line);
    m_selection.set_start_index(dest_index);
    m_selection.set_end_line(old_end_line);
    m_selection.set_end_index(old_end_index);
}

void Document::split_line_at_cursor() {
    push_command<InsertCommand>("\n");
}

bool Document::execute_command(Command& command) {
    scroll_cursor_into_view();
    return command.execute();
}

void Document::redo() {
    if (m_command_stack_index == m_command_stack.size()) {
        return;
    }

    auto& command = *m_command_stack[m_command_stack_index++];
    command.redo();

    if (on_change) {
        on_change();
    }
}

void Document::undo() {
    if (m_command_stack_index == 0) {
        return;
    }

    auto& command = *m_command_stack[--m_command_stack_index];
    command.undo();
    update_search_results();
    update_syntax_highlighting();

    if (on_change) {
        on_change();
    }
}

Document::StateSnapshot Document::snapshot_state() const {
    return { cursor_row_position(), cursor_col_position(), m_max_cursor_col, m_document_was_modified, m_selection };
}

Document::Snapshot Document::snapshot() const {
    return { Vector<Line>(m_lines), snapshot_state() };
}

void Document::restore(Snapshot s) {
    m_lines = move(s.lines);
    m_max_cursor_col = s.state.max_cursor_col;
    m_document_was_modified = s.state.document_was_modified;
    m_selection = s.state.selection;

    update_search_results();
    m_panel.set_cursor(s.state.absolute_cursor_row - m_row_offset, s.state.absolute_cursor_col - m_col_offset);
    scroll_cursor_into_view();
    set_needs_display();
}

void Document::restore_state(const StateSnapshot& s) {
    m_max_cursor_col = s.max_cursor_col;
    m_document_was_modified = s.document_was_modified;

    clear_selection();
    m_selection = s.selection;
    render_selection();

    m_panel.set_cursor(s.absolute_cursor_row - m_row_offset, s.absolute_cursor_col - m_col_offset);
    scroll_cursor_into_view();
    set_needs_display();
}

void Document::insert_text_at_cursor(const String& text) {
    if (text.is_empty()) {
        return;
    }

    push_command<InsertCommand>(text);
}

void Document::render_selection() {
    if (m_selection.empty()) {
        return;
    }

    int line_start = m_selection.upper_line();
    int index_start = m_selection.upper_index();
    int line_end = m_selection.lower_line();
    int index_end = m_selection.lower_index();

    for (int li = line_start; li <= line_end; li++) {
        auto& line = m_lines[li];

        int si = 0;
        if (li == line_start) {
            si = index_start;
        }

        int ei = line.length();
        if (li == line_end) {
            ei = index_end;
        }

        if (si == 0 && ei == line.length()) {
            line.select_all();
            continue;
        }

        for (int i = si; i < ei; i++) {
            line.metadata_at(i).set_selected(true);
        }
    }

    set_needs_display();
}

void Document::move_cursor_to(int line_index, int index_into_line, MovementMode mode) {
    while (cursor_row_position() < line_index) {
        move_cursor_down(mode);
    }
    while (cursor_row_position() > line_index) {
        move_cursor_up(mode);
    }

    while (line_index_at_cursor() < index_into_line) {
        move_cursor_right(mode);
    }
    while (line_index_at_cursor() > index_into_line) {
        move_cursor_left(mode);
    }
}

void Document::delete_selection() {
    int line_start = m_selection.upper_line();
    int index_start = m_selection.upper_index();
    int line_end = m_selection.lower_line();
    int index_end = m_selection.lower_index();
    m_selection.clear();

    move_cursor_to(line_start, index_start);

    for (int li = line_end; li >= line_start; li--) {
        auto& line = m_lines[li];

        int si = 0;
        if (li == line_start) {
            si = index_start;
        }

        int ei = line.length();
        if (li == line_end) {
            ei = index_end;
        }

        if (si == 0 && ei == line.length() && li != line_end) {
            remove_line(li);
            continue;
        }

        for (int i = ei - 1; i >= si; i--) {
            line.remove_char_at(i);
        }
    }

    set_needs_display();
    m_document_was_modified = true;
}

String Document::selection_text() const {
    if (m_selection.empty()) {
        return "";
    }

    int line_start = m_selection.upper_line();
    int index_start = m_selection.upper_index();
    int line_end = m_selection.lower_line();
    int index_end = m_selection.lower_index();

    String result;
    for (int li = line_start; li <= line_end; li++) {
        auto& line = m_lines[li];

        if (li != line_start) {
            result += "\n";
        }

        int si = 0;
        if (li == line_start) {
            si = index_start;
        }

        int ei = line.length();
        if (li == line_end) {
            ei = index_end;
        }

        if (si == 0 && ei == line.length()) {
            result += line.contents();
            continue;
        }

        for (int i = si; i < ei; i++) {
            result += String(line.char_at(i));
        }
    }

    return result;
}

void Document::clear_selection() {
    if (!m_selection.empty()) {
        m_selection.clear();
        for (auto& line : m_lines) {
            line.clear_selection();
        }
        set_needs_display();
    }
}

void Document::remove_line(int index) {
    m_lines.remove(index);
    m_panel.notify_line_count_changed();

    int last_row_index = m_lines.size();
    int row_in_panel = last_row_index - m_row_offset;
    if (row_in_panel >= 0 && row_in_panel < m_panel.rows()) {
        for (int c = 0; c < m_panel.cols(); c++) {
            m_panel.set_text_at(row_in_panel, c, ' ', {});
        }
    }

    set_needs_display();
}

void Document::insert_line(Line&& line, int index) {
    m_lines.insert(move(line), index);
    m_panel.notify_line_count_changed();
}

void Document::rotate_lines_up(int start, int end) {
    // Vector::rotate is exclusive, this is inclusive
    m_lines.rotate_left(start, end + 1);
}

void Document::rotate_lines_down(int start, int end) {
    // Vector::rotate is exclusive, this is inclusive
    m_lines.rotate_right(start, end + 1);
}

void Document::copy() {
    if (m_selection.empty()) {
        String contents = line_at_cursor().contents();
        if (!single_line_mode()) {
            contents += "\n";
        }
        m_panel.set_clipboard_contents(move(contents), true);
        return;
    }

    m_panel.set_clipboard_contents(selection_text());
}

void Document::cut() {
    if (m_selection.empty()) {
        auto contents = line_at_cursor().contents();
        if (!single_line_mode()) {
            contents += "\n";
        }
        m_panel.set_clipboard_contents(move(contents), true);
        push_command<DeleteLineCommand>();
        return;
    }

    m_panel.set_clipboard_contents(selection_text());
    push_command<DeleteCommand>(DeleteCharMode::Delete);
}

void Document::paste() {
    bool is_whole_line;
    auto text_to_insert = m_panel.clipboard_contents(is_whole_line);
    if (text_to_insert.is_empty()) {
        return;
    }

    if (!single_line_mode() && m_selection.empty() && is_whole_line) {
        text_to_insert.remove_index(text_to_insert.size() - 1);
        push_command<InsertLineCommand>(text_to_insert);
    } else {
        insert_text_at_cursor(text_to_insert);
    }

    set_needs_display();
}

void Document::set_show_line_numbers(bool b) {
    if (m_show_line_numbers != b) {
        m_show_line_numbers = b;
        m_panel.notify_line_count_changed();
    }
}

void Document::notify_panel_size_changed() {
    while (m_panel.cursor_row() >= m_panel.rows()) {
        move_cursor_up();
    }

    while (m_panel.cursor_col() >= m_panel.cols()) {
        move_cursor_left();
    }

    display();
}

void Document::go_to_line() {
    auto maybe_result = m_panel.prompt("Go to line: ");
    if (!maybe_result.has_value()) {
        return;
    }

    auto& result = maybe_result.value();
    char* end_ptr = result.string();
    long line_number = strtol(result.string(), &end_ptr, 10);
    if (errno == ERANGE || end_ptr != result.string() + result.size() || line_number < 1 || line_number > num_lines()) {
        m_panel.send_status_message(String::format("Line `%s' is not between 1 and %d", result.string(), num_lines()));
        return;
    }

    clear_selection();

    int screen_midpoint = m_panel.rows() / 2;
    if (line_number - 1 < screen_midpoint) {
        m_panel.set_cursor_row(line_number - 1);
        m_row_offset = 0;
    } else {
        m_panel.set_cursor_row(screen_midpoint);
        m_row_offset = line_number - 1 - screen_midpoint;
    }

    move_cursor_to_line_start();
    set_needs_display();
}

void Document::set_type(DocumentType type) {
    if (type == m_type) {
        return;
    }

    m_type = type;
    update_syntax_highlighting();
}

void Document::guess_type_from_name() {
    update_document_type(*this);
}

void Document::save() {
    if (m_name.is_empty()) {
        auto result = m_panel.prompt("Save as: ");
        if (!result.has_value()) {
            return;
        }

        if (access(result.value().string(), F_OK) == 0) {
            auto ok = m_panel.prompt(String::format("Are you sure you want to overwrite file `%s'? ", result.value().string()));
            if (!ok.has_value() || (ok.value() != "y" && ok.value() != "yes")) {
                return;
            }
        }

        m_name = move(result.value());
        guess_type_from_name();
    }

    if (access(m_name.string(), W_OK)) {
        if (errno != ENOENT) {
            m_panel.send_status_message(String::format("Permission to write file `%s' denied", m_name.string()));
            return;
        }
    }

    FILE* file = fopen(m_name.string(), "w");
    if (!file) {
        m_panel.send_status_message(String::format("Failed to save - `%s'", strerror(errno)));
        return;
    }

    if (m_lines.size() != 1 || !m_lines.first().empty()) {
        for (auto& line : m_lines) {
            fprintf(file, "%s\n", line.contents().string());
        }
    }

    if (ferror(file)) {
        m_panel.send_status_message(String::format("Failed to write to disk - `%s'", strerror(errno)));
        fclose(file);
        return;
    }

    if (fclose(file)) {
        m_panel.send_status_message(String::format("Failed to sync to disk - `%s'", strerror(errno)));
        return;
    }

    m_panel.send_status_message(String::format("Successfully saved file: `%s'", m_name.string()));
    m_document_was_modified = false;
}

void Document::quit() {
    if (m_document_was_modified && !single_line_mode()) {
        auto result = m_panel.prompt("Quit without saving? ");
        if (!result.has_value() || (result.value() != "y" && result.value() != "yes")) {
            return;
        }
    }

    m_panel.quit();
}

void Document::update_syntax_highlighting() {
    for (auto& line : m_lines) {
        line.clear_syntax_highlighting();
    }
    highlight_document(*this);
}

void Document::update_search_results() {
    clear_search_results();
    m_search_result_count = 0;
    if (m_search_text.is_empty()) {
        return;
    }

    for (auto& line : m_lines) {
        int num = line.search(m_search_text);
        m_search_result_count += num;
        if (num > 0) {
            set_needs_display();
        }
    }
}

void Document::clear_search_results() {
    if (m_search_result_count == 0) {
        return;
    }

    for (auto& line : m_lines) {
        line.clear_search();
    }
    set_needs_display();
}

void Document::set_search_text(String text) {
    if (m_search_text == text) {
        return;
    }

    m_search_text = move(text);
    update_search_results();
}

void Document::move_cursor_to_next_search_match() {
    if (m_search_result_count <= 1) {
        return;
    }

    for (;;) {
        move_cursor_right();
        if (cursor_at_document_end()) {
            move_cursor_to_document_start();
        }

        auto& line = line_at_cursor();
        int line_index = line_index_at_cursor();
        if (strstr(line.contents().string() + line_index, m_search_text.string()) == line.contents().string() + line_index) {
            for (size_t i = 0; i < m_search_text.size(); i++) {
                move_cursor_right(MovementMode::Select);
            }
            return;
        }
    }
}

void Document::enter_interactive_search() {
    m_panel.enter_search(m_search_text);
    m_panel.send_status_message(String::format("Found %d result(s)", m_search_result_count));
}

void Document::swap_lines_at_cursor(SwapDirection direction) {
    push_command<SwapLinesCommand>(direction);
}

void Document::select_word_at_cursor() {
    bool was_space = isspace(char_at_cursor());
    bool was_word = isword(char_at_cursor());
    while (line_index_at_cursor() > 0) {
        move_cursor_left(MovementMode::Move);
        if (isspace(char_at_cursor()) != was_space || isword(char_at_cursor()) != was_word) {
            move_cursor_right(MovementMode::Move);
            break;
        }
    }

    while (line_index_at_cursor() < line_at_cursor().length()) {
        move_cursor_right(MovementMode::Select);
        if (isspace(char_at_cursor()) != was_space || isword(char_at_cursor()) != was_word) {
            break;
        }
    }
}

void Document::select_line_at_cursor() {
    move_cursor_to_line_start(MovementMode::Move);
    move_cursor_down(MovementMode::Select);
}

bool Document::notify_mouse_event(MouseEvent event) {
    bool handled = false;
    if (event.left == MouseEvent::Press::Down) {
        move_cursor_to(event.index_of_line, event.index_into_line, MovementMode::Move);
        handled = true;
    } else if (event.left == MouseEvent::Press::Double) {
        move_cursor_to(event.index_of_line, event.index_into_line, MovementMode::Move);
        select_word_at_cursor();
        handled = true;
    } else if (event.left == MouseEvent::Press::Triple) {
        move_cursor_to(event.index_of_line, event.index_into_line, MovementMode::Move);
        select_line_at_cursor();
        handled = true;
    } else if (event.down & MouseEvent::Button::Left) {
        move_cursor_to(event.index_of_line, event.index_into_line, MovementMode::Select);
        handled = true;
    } else if (event.z != 0) {
        scroll(2 * event.z);
        handled = true;
    }

    if (needs_display()) {
        display();
    } else {
        m_panel.notify_now_is_a_good_time_to_draw_cursor();
    }
    return handled;
}

void Document::notify_key_pressed(KeyPress press) {
    if (press.modifiers & KeyPress::Modifier::Alt) {
        switch (press.key) {
            case KeyPress::Key::DownArrow:
                swap_lines_at_cursor(SwapDirection::Down);
                break;
            case KeyPress::Key::UpArrow:
                swap_lines_at_cursor(SwapDirection::Up);
                break;
            default:
                break;
        }

        if (needs_display()) {
            display();
        } else {
            m_panel.notify_now_is_a_good_time_to_draw_cursor();
        }
        return;
    }

    if (press.modifiers & KeyPress::Modifier::Control) {
        switch (toupper(press.key)) {
            case KeyPress::Key::LeftArrow:
                move_cursor_left_by_word(press.modifiers & KeyPress::Modifier::Shift ? MovementMode::Select : MovementMode::Move);
                break;
            case KeyPress::Key::RightArrow:
                move_cursor_right_by_word(press.modifiers & KeyPress::Modifier::Shift ? MovementMode::Select : MovementMode::Move);
                break;
            case KeyPress::Key::DownArrow:
                move_cursor_down(press.modifiers & KeyPress::Modifier::Shift ? MovementMode::Select : MovementMode::Move);
                break;
            case KeyPress::Key::UpArrow:
                move_cursor_up(press.modifiers & KeyPress::Modifier::Shift ? MovementMode::Select : MovementMode::Move);
                break;
            case KeyPress::Key::Home:
                move_cursor_to_document_start(press.modifiers & KeyPress::Modifier::Shift ? MovementMode::Select : MovementMode::Move);
                break;
            case KeyPress::Key::End:
                move_cursor_to_document_end(press.modifiers & KeyPress::Modifier::Shift ? MovementMode::Select : MovementMode::Move);
                break;
            case KeyPress::Key::Backspace:
                delete_word(DeleteCharMode::Backspace);
                break;
            case KeyPress::Key::Delete:
                delete_word(DeleteCharMode::Delete);
                break;
            case 'C':
                copy();
                break;
            case 'F':
                enter_interactive_search();
                break;
            case 'G':
                go_to_line();
                break;
            case 'L':
                if (!single_line_mode()) {
                    set_show_line_numbers(!m_show_line_numbers);
                }
                break;
            case 'O':
                if (!single_line_mode()) {
                    m_panel.do_open_prompt();
                }
                break;
            case 'Q':
            case 'W':
                quit();
                break;
            case 'S':
                if (!single_line_mode()) {
                    save();
                }
                break;
            case 'V':
                paste();
                break;
            case 'X':
                cut();
                break;
            case 'Y':
                redo();
                break;
            case 'Z':
                undo();
                break;
            default:
                break;
        }

        if (needs_display()) {
            display();
        } else {
            m_panel.notify_now_is_a_good_time_to_draw_cursor();
        }
        return;
    }

    switch (press.key) {
        case KeyPress::Key::LeftArrow:
            move_cursor_left(press.modifiers & KeyPress::Modifier::Shift ? MovementMode::Select : MovementMode::Move);
            break;
        case KeyPress::Key::RightArrow:
            move_cursor_right(press.modifiers & KeyPress::Modifier::Shift ? MovementMode::Select : MovementMode::Move);
            break;
        case KeyPress::Key::DownArrow:
            move_cursor_down(press.modifiers & KeyPress::Modifier::Shift ? MovementMode::Select : MovementMode::Move);
            break;
        case KeyPress::Key::UpArrow:
            move_cursor_up(press.modifiers & KeyPress::Modifier::Shift ? MovementMode::Select : MovementMode::Move);
            break;
        case KeyPress::Key::Home:
            move_cursor_to_line_start(press.modifiers & KeyPress::Modifier::Shift ? MovementMode::Select : MovementMode::Move);
            break;
        case KeyPress::Key::End:
            move_cursor_to_line_end(press.modifiers & KeyPress::Modifier::Shift ? MovementMode::Select : MovementMode::Move);
            break;
        case KeyPress::Key::PageUp:
            move_cursor_page_up(press.modifiers & KeyPress::Modifier::Shift ? MovementMode::Select : MovementMode::Move);
            break;
        case KeyPress::Key::PageDown:
            move_cursor_page_down(press.modifiers & KeyPress::Modifier::Shift ? MovementMode::Select : MovementMode::Move);
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
            } else if (on_submit) {
                on_submit();
            }
            break;
        case KeyPress::Key::Escape:
            m_search_text = "";
            clear_search_results();
            clear_selection();
            if (on_escape_press) {
                on_escape_press();
            }
            break;
        default:
            if (isascii(press.key)) {
                insert_char(press.key);
            }
            break;
    }

    if (needs_display()) {
        display();
    } else {
        m_panel.notify_now_is_a_good_time_to_draw_cursor();
    }
}
