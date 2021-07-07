#include <edit/command.h>
#include <edit/document.h>
#include <edit/key_press.h>
#include <edit/panel.h>
#include <edit/position.h>
#include <errno.h>
#include <eventloop/event.h>
#include <ext/file.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

namespace Edit {
static inline int isword(int c) {
    return isalnum(c) || c == '_';
}

UniquePtr<Document> Document::create_from_stdin(const String& path, Panel& panel) {
    auto file = Ext::File(stdin);
    file.set_should_close_file(false);

    Vector<Line> lines;
    auto result = file.read_all_lines(
        [&](auto line_string) -> bool {
            lines.add(Line(move(line_string)));
            return true;
        },
        Ext::StripTrailingNewlines::Yes);

    UniquePtr<Document> ret;

    if (!result) {
        panel.send_status_message(String::format("error reading stdin: `%s'", strerror(file.error())));
        ret = Document::create_empty(panel);
        ret->set_name(path);
    } else {
        lines.add(Line(""));
        ret = make_unique<Document>(move(lines), path, panel, InputMode::Document);
    }

    assert(freopen("/dev/tty", "r+", stdin));
    return ret;
}

UniquePtr<Document> Document::create_from_file(const String& path, Panel& panel) {
    auto file = Ext::File::create(path, "r");
    if (!file) {
        if (errno == ENOENT) {
            panel.send_status_message(String::format("new file: `%s'", path.string()));
            return make_unique<Document>(Vector<Line>(), path, panel, InputMode::Document);
        }
        panel.send_status_message(String::format("error accessing file: `%s': `%s'", path.string(), strerror(errno)));
        return Document::create_empty(panel);
    }

    Vector<Line> lines;
    auto result = file->read_all_lines(
        [&](auto line_string) -> bool {
            lines.add(Line(move(line_string)));
            return true;
        },
        Ext::StripTrailingNewlines::Yes);

    UniquePtr<Document> ret;

    if (!result) {
        panel.send_status_message(String::format("error reading file: `%s': `%s'", path.string(), strerror(file->error())));
        ret = Document::create_empty(panel);
    } else {
        lines.add(Line(""));
        ret = make_unique<Document>(move(lines), path, panel, InputMode::Document);
    }

    if (!file->close()) {
        panel.send_status_message(String::format("error closing file: `%s'", path.string()));
    }

    return ret;
}

UniquePtr<Document> Document::create_from_text(Panel& panel, const String& text) {
    auto lines_view = text.split_view('\n');

    Vector<Line> lines(lines_view.size());
    for (auto& line_view : lines_view) {
        lines.add(Line(String(line_view)));
    }

    return make_unique<Document>(move(lines), "", panel, InputMode::InputText);
}

UniquePtr<Document> Document::create_empty(Panel& panel) {
    return make_unique<Document>(Vector<Line>(), "", panel, InputMode::Document);
}

UniquePtr<Document> Document::create_single_line(Panel& panel, String text) {
    Vector<Line> lines;
    lines.add(Line(move(text)));
    auto ret = make_unique<Document>(move(lines), "", panel, InputMode::InputText);
    ret->set_submittable(true);
    ret->set_show_line_numbers(false);
    ret->move_cursor_to_line_end();
    ret->select_all();
    return ret;
}

Document::Document(Vector<Line> lines, String name, Panel& panel, InputMode mode)
    : m_lines(move(lines))
    , m_name(move(name))
    , m_input_mode(mode)
    , m_search_results(*this)
    , m_syntax_highlighting_info(*this)
    , m_panel(panel) {
    if (m_lines.empty()) {
        m_lines.add(Line(""));
    }
    guess_type_from_name();
}

Document::~Document() {}

void Document::copy_settings_from(const Document& other) {
    set_input_mode(other.m_input_mode);
    set_submittable(other.m_submittable);
    set_type(other.m_type);

    set_auto_complete_mode(other.m_auto_complete_mode);
    set_preview_auto_complete(other.m_preview_auto_complete);
    set_convert_tabs_to_spaces(other.m_convert_tabs_to_spaces);
    set_word_wrap_enabled(other.m_word_wrap_enabled);

    set_show_line_numbers(other.show_line_numbers());
}

String Document::content_string() const {
    if (input_text_mode() && num_lines() == 1) {
        return m_lines.first().contents();
    }

    String ret;
    for (auto& line : m_lines) {
        ret += line.contents();
        if (!input_text_mode() || &line != &last_line()) {
            ret += "\n";
        }
    }
    return ret;
}

size_t Document::cursor_index_in_content_string() const {
    if (input_text_mode() && num_lines() == 1) {
        return m_cursor.index_into_line();
    }

    size_t index = 0;
    for (int i = 0; i < m_cursor.line_index(); i++) {
        index += line_at_index(i).length() + 1;
    }

    index += m_cursor.index_into_line();
    return index;
}

void Document::display_if_needed() const {
    if (needs_display()) {
        display();
    }
}

void Document::display() const {
    auto& document = const_cast<Document&>(*this);

    auto render_index = text_index_at_absolute_position({ m_row_offset, 0 });
    auto relative_start_position =
        line_at_index(render_index.line_index()).relative_position_of_index(*this, m_panel, render_index.index_into_line());
    render_index.set_index_into_line(0);

    TextRangeCollection selection_collection(*this);
    selection_collection.add(cursor().selection().text_range());
    DocumentTextRangeIterator metadata_iterator(render_index, m_syntax_highlighting_info, m_search_results, selection_collection);

    int row = m_row_offset;
    for (; render_index.line_index() < num_lines() && row < m_row_offset + m_panel.rows();) {
        auto& line = line_at_index(render_index.line_index());
        row += line.render(document, document.panel(), metadata_iterator, m_col_offset, relative_start_position.row, row - m_row_offset);
        render_index.set_line_index(render_index.line_index() + 1);
        relative_start_position = { 0, 0 };
    }

    // Explicitly clear any visible line after the document ends
    for (; row - m_row_offset < m_panel.rows(); row++) {
        for (int col = 0; col < m_panel.cols(); col++) {
            m_panel.set_text_at(row - m_row_offset, col, ' ', {});
        }
    }

    m_panel.flush();
    m_needs_display = false;
}

TextIndex Document::text_index_at_absolute_position(const Position& position) const {
    if (position.row < 0) {
        return { 0, 0 };
    }

    int absolute_row = 0;
    for (auto& line : m_lines) {
        auto height_of_line = line.rendered_line_count(*this, m_panel);
        if (position.row < absolute_row + height_of_line) {
            return { index_of_line(line), line.index_of_relative_position(*this, m_panel, { position.row - absolute_row, position.col }) };
        }
        absolute_row += height_of_line;
    }

    return { num_lines() - 1, last_line().length() };
}

TextIndex Document::text_index_at_scrolled_position(const Position& position) const {
    return text_index_at_absolute_position({ position.row + m_row_offset, position.col + m_col_offset });
}

Position Document::relative_to_absolute_position(const Panel& panel, const Line& reference_line,
                                                 const Position& line_relative_position) const {
    return { reference_line.absolute_row_position(*this, panel) + line_relative_position.row, line_relative_position.col };
}

Line& Document::line_at_cursor() {
    return m_cursor.referenced_line(*this);
}

char Document::char_at_cursor() const {
    return m_cursor.referenced_character(*this);
}

int Document::index_of_line_at_cursor() const {
    return m_cursor.line_index();
}

int Document::index_into_line_at_cursor() const {
    return m_cursor.index_into_line();
}

const TextIndex& Document::index_at_cursor() const {
    return m_cursor.index();
}

int Document::num_rendered_lines() const {
    int total = 0;
    for (auto& line : m_lines) {
        total += line.rendered_line_count(*this, m_panel);
    }
    return total;
}

Position Document::cursor_position_on_panel() const {
    auto position = m_cursor.absolute_position(*this, m_panel);
    return { position.row - m_row_offset, position.col - m_col_offset };
}

bool Document::cursor_at_document_start() const {
    return m_cursor.at_document_start(*this);
}

bool Document::cursor_at_document_end() const {
    return m_cursor.at_document_end(*this);
}

int Document::index_of_line(const Line& line) const {
    return &line - m_lines.vector();
}

void Document::update_selection_state_for_mode(MovementMode mode) {
    if (mode == MovementMode::Move) {
        clear_selection(cursor());
        return;
    }

    if (cursor().selection().empty()) {
        cursor().selection().begin(index_at_cursor());
    }
}

void Document::move_cursor_right_by_word(MovementMode mode) {
    move_cursor_right(mode);

    auto& line = line_at_cursor();
    while (index_into_line_at_cursor() < line.length() && !isword(char_at_cursor())) {
        move_cursor_right(mode);
    }

    while (index_into_line_at_cursor() < line.length() && isword(char_at_cursor())) {
        move_cursor_right(mode);
    }
}

void Document::move_cursor_left_by_word(MovementMode mode) {
    move_cursor_left(mode);

    while (index_into_line_at_cursor() > 0 && !isword(char_at_cursor())) {
        move_cursor_left(mode);
    }

    bool found_word = false;
    while (index_into_line_at_cursor() > 0 && isword(char_at_cursor())) {
        move_cursor_left(mode);
        found_word = true;
    }

    if (found_word && !isword(char_at_cursor())) {
        move_cursor_right(mode);
    }
}

void Document::move_cursor_right(MovementMode mode) {
    auto& line = line_at_cursor();
    int index_into_line = index_into_line_at_cursor();
    if (index_into_line == line.length()) {
        if (&line == &m_lines.last()) {
            return;
        }

        move_cursor_down(mode);
        move_cursor_to_line_start(mode);
        return;
    }

    auto& selection = cursor().selection();
    if (!selection.empty() && mode == MovementMode::Move) {
        auto selection_end = selection.normalized_end();
        clear_selection(cursor());
        move_cursor_to(selection_end);
        return;
    }

    int new_index_into_line = m_cursor.index_into_line() + 1;
    int new_col_position = line.relative_position_of_index(*this, m_panel, new_index_into_line).col;

    cursor().set_max_col(new_col_position);

    if (mode == MovementMode::Select) {
        if (selection.empty()) {
            selection.begin(index_at_cursor());
        }
        selection.set_end_index_into_line(new_index_into_line);
        set_needs_display();
    }

    m_cursor.set_index_into_line(new_index_into_line);
}

void Document::move_cursor_left(MovementMode mode) {
    auto& line = line_at_cursor();
    int index_into_line = index_into_line_at_cursor();
    if (index_into_line == 0) {
        if (&line == &m_lines.first()) {
            return;
        }

        move_cursor_up(mode);
        move_cursor_to_line_end(mode);
        return;
    }

    auto& selection = cursor().selection();
    if (!selection.empty() && mode == MovementMode::Move) {
        auto selection_start = selection.normalized_start();
        clear_selection(cursor());
        move_cursor_to(selection_start);
        return;
    }

    int new_index_into_line = index_into_line - 1;
    int new_col_position = line.relative_position_of_index(*this, m_panel, new_index_into_line).col;

    cursor().set_max_col(new_col_position);

    if (mode == MovementMode::Select) {
        if (selection.empty()) {
            selection.begin(index_at_cursor());
        }
        selection.set_end_index_into_line(new_index_into_line);
        set_needs_display();
    }

    m_cursor.set_index_into_line(new_index_into_line);
}

void Document::move_cursor_down(MovementMode mode) {
    auto& prev_line = line_at_cursor();
    if (&prev_line == &last_line() && last_line().relative_position_of_index(*this, m_panel, index_into_line_at_cursor()).row ==
                                          last_line().rendered_line_count(*this, m_panel) - 1) {
        move_cursor_to_line_end(mode);
        return;
    }

    update_selection_state_for_mode(mode);

    auto prev_position = m_cursor.absolute_position(*this, m_panel);
    auto new_index = text_index_at_absolute_position({ prev_position.row + 1, prev_position.col });

    m_cursor.set(new_index);

    clamp_cursor_to_line_end();
    if (mode == MovementMode::Select) {
        cursor().selection().set_end(index_at_cursor());
        set_needs_display();
    }
}

void Document::move_cursor_up(MovementMode mode) {
    auto& prev_line = line_at_cursor();
    if (&prev_line == &first_line() && first_line().relative_position_of_index(*this, m_panel, index_into_line_at_cursor()).row == 0) {
        move_cursor_to_line_start(mode);
        return;
    }

    update_selection_state_for_mode(mode);

    auto prev_position = m_cursor.absolute_position(*this, m_panel);
    auto new_index = text_index_at_absolute_position({ prev_position.row - 1, prev_position.col });

    m_cursor.set(new_index);

    clamp_cursor_to_line_end();
    if (mode == MovementMode::Select) {
        cursor().selection().set_end(index_at_cursor());
        set_needs_display();
    }
}

void Document::clamp_cursor_to_line_end() {
    auto& line = line_at_cursor();
    auto current_pos = m_cursor.relative_position(*this, m_panel);
    auto max_col = line.max_col_in_relative_row(*this, m_panel, current_pos.row);
    if (current_pos.col == max_col) {
        return;
    }

    if (current_pos.col > max_col) {
        m_cursor.set_index_into_line(line.index_of_relative_position(*this, m_panel, { current_pos.row, max_col }));
        return;
    }

    if (cursor().max_col() > current_pos.col) {
        m_cursor.set_index_into_line(line.index_of_relative_position(*this, m_panel, { current_pos.row, cursor().max_col() }));
        return;
    }
}

void Document::move_cursor_to_line_start(MovementMode mode) {
    update_selection_state_for_mode(mode);
    if (mode == MovementMode::Select) {
        cursor().selection().set_end_index_into_line(0);
        set_needs_display();
    }

    m_cursor.set_index_into_line(0);
    m_cursor.set_max_col(0);
}

void Document::move_cursor_to_line_end(MovementMode mode) {
    auto& line = line_at_cursor();
    auto new_col = line.relative_position_of_index(*this, m_panel, line.length()).col;

    cursor().set_max_col(new_col);

    update_selection_state_for_mode(mode);
    if (mode == MovementMode::Select) {
        cursor().selection().set_end_index_into_line(line.length());
        set_needs_display();
    }

    m_col_offset = 0;
    m_cursor.set_index_into_line(line.length());
}

void Document::move_cursor_to_document_start(MovementMode mode) {
    move_cursor_to({ 0, 0 }, mode);
}

void Document::move_cursor_to_document_end(MovementMode mode) {
    auto last_line_index = m_lines.size() - 1;
    auto& last_line = m_lines.last();
    move_cursor_to({ last_line_index, last_line.length() }, mode);
}

void Document::scroll(int vertical, int horizontal) {
    if (vertical < 0) {
        scroll_up(-vertical);
    } else if (vertical > 0) {
        scroll_down(vertical);
    }

    if (horizontal < 0) {
        scroll_left(-horizontal);
    } else if (horizontal > 0) {
        scroll_right(horizontal);
    }
}

void Document::scroll_up(int times) {
    for (int i = 0; i < times; i++) {
        if (m_row_offset > 0) {
            m_row_offset--;
            set_needs_display();
        }
    }
}

void Document::scroll_down(int times) {
    for (int i = 0; i < times; i++) {
        if (m_row_offset + m_panel.rows() < num_rendered_lines()) {
            m_row_offset++;
            set_needs_display();
        }
    }
}

void Document::scroll_left(int times) {
    for (int i = 0; i < times; i++) {
        if (m_col_offset > 0) {
            m_col_offset--;
            set_needs_display();
        }
    }
}

void Document::scroll_right(int times) {
    for (int i = 0; i < times; i++) {
        m_col_offset++;
        set_needs_display();
    }
}

void Document::scroll_cursor_into_view() {
    if (cursor_position_on_panel().row < 0) {
        scroll_up(-cursor_position_on_panel().row);
    } else if (cursor_position_on_panel().row >= m_panel.rows()) {
        scroll_down(cursor_position_on_panel().row - m_panel.rows() + 1);
    }

    if (cursor_position_on_panel().col < 0) {
        scroll_left(-cursor_position_on_panel().col);
    } else if (cursor_position_on_panel().col >= m_panel.cols()) {
        scroll_right(cursor_position_on_panel().col - m_panel.cols() + 1);
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
    if (!cursor().selection().empty()) {
        delete_char(mode);
        return;
    }

    int index_into_line = index_into_line_at_cursor();
    if ((mode == DeleteCharMode::Backspace && index_into_line == 0) ||
        (mode == DeleteCharMode::Delete && index_into_line == line_at_cursor().length())) {
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
    auto& selection = cursor().selection();
    auto start = selection.start();
    auto end = selection.end();

    move_cursor_to(start);

    selection.set(start, end);
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
    return { m_cursor, m_document_was_modified };
}

Document::Snapshot Document::snapshot() const {
    return { Vector<Line>(m_lines), snapshot_state() };
}

void Document::restore(Snapshot s) {
    m_lines = move(s.lines);
    m_cursor = s.state.cursor;
    m_document_was_modified = s.state.document_was_modified;

    update_search_results();
    scroll_cursor_into_view();
    set_needs_display();
}

void Document::restore_state(const StateSnapshot& s) {
    m_cursor = s.cursor;
    m_document_was_modified = s.document_was_modified;

    scroll_cursor_into_view();
    set_needs_display();
}

void Document::insert_text_at_cursor(const String& text) {
    if (text.is_empty()) {
        return;
    }

    push_command<InsertCommand>(text);
}

void Document::move_cursor_to(const TextIndex& index, MovementMode mode) {
    while (index_of_line_at_cursor() < index.line_index()) {
        move_cursor_down(mode);
    }
    while (index_of_line_at_cursor() > index.line_index()) {
        move_cursor_up(mode);
    }

    while (index_into_line_at_cursor() < index.index_into_line()) {
        move_cursor_right(mode);
    }
    while (index_into_line_at_cursor() > index.index_into_line()) {
        move_cursor_left(mode);
    }
}

void Document::delete_selection(Cursor& cursor) {
    auto& selection = cursor.selection();
    auto start = selection.normalized_start();
    auto end = selection.normalized_end();

    auto line_start = start.line_index();
    auto index_start = start.index_into_line();
    auto line_end = end.line_index();
    auto index_end = end.index_into_line();

    clear_selection(cursor);
    move_cursor_to(start);

    if (line_start == line_end) {
        for (int i = index_end - 1; i >= index_start; i--) {
            m_lines[line_start].remove_char_at(i);
        }
    } else {
        auto split_end = m_lines[line_end].split_at(index_end);
        remove_line(line_end);

        for (int i = line_end - 1; i > line_start; i--) {
            remove_line(i);
        }

        auto split_start = m_lines[line_start].split_at(index_start);
        m_lines[line_start] = split_start.first;
        m_lines[line_start].combine_line(split_end.second);
    }

    set_needs_display();
    m_document_was_modified = true;
}

String Document::selection_text(Cursor& cursor) const {
    auto& selection = cursor.selection();
    if (selection.empty()) {
        return "";
    }

    auto start = selection.normalized_start();
    auto end = selection.normalized_end();

    String result;
    for (int li = start.line_index(); li <= end.line_index(); li++) {
        auto& line = m_lines[li];

        if (li != start.line_index()) {
            result += "\n";
        }

        int si = 0;
        if (li == start.line_index()) {
            si = start.index_into_line();
        }

        int ei = line.length();
        if (li == end.line_index()) {
            ei = end.index_into_line();
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

void Document::clear_selection(Cursor& cursor) {
    if (!cursor.selection().empty()) {
        cursor.selection().clear();
        set_needs_display();
    }
}

void Document::remove_line(int index) {
    m_lines.remove(index);
    m_panel.notify_line_count_changed();

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
    if (cursor().selection().empty()) {
        String contents = line_at_cursor().contents();
        if (!input_text_mode()) {
            contents += "\n";
        }
        m_panel.set_clipboard_contents(move(contents), true);
        return;
    }

    m_panel.set_clipboard_contents(selection_text(cursor()));
}

void Document::cut() {
    if (cursor().selection().empty()) {
        auto contents = line_at_cursor().contents();
        if (!input_text_mode()) {
            contents += "\n";
        }
        m_panel.set_clipboard_contents(move(contents), true);
        push_command<DeleteLineCommand>();
        return;
    }

    m_panel.set_clipboard_contents(selection_text(cursor()));
    push_command<DeleteCommand>(DeleteCharMode::Delete);
}

void Document::paste() {
    bool is_whole_line;
    auto text_to_insert = m_panel.clipboard_contents(is_whole_line);
    if (text_to_insert.is_empty()) {
        return;
    }

    if (!input_text_mode() && cursor().selection().empty() && is_whole_line) {
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
    if (word_wrap_enabled()) {
        for (auto& line : m_lines) {
            line.invalidate_rendered_contents();
        }
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

    clear_selection(cursor());
    m_cursor.set_line_index(line_number - 1);

    auto cursor_row_position = line_at_cursor().absolute_row_position(*this, m_panel);

    int screen_midpoint = m_panel.rows() / 2;
    if (cursor_row_position < screen_midpoint) {
        m_row_offset = 0;
    } else {
        m_row_offset = cursor_row_position - screen_midpoint;
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
    if (m_document_was_modified && !input_text_mode()) {
        auto result = m_panel.prompt("Quit without saving? ");
        if (!result.has_value() || (result.value() != "y" && result.value() != "yes")) {
            return;
        }
    }

    m_panel.quit();
}

void Document::update_syntax_highlighting() {
    highlight_document(*this);
}

void Document::update_search_results() {
    clear_search_results();
    if (m_search_text.is_empty()) {
        return;
    }

    for (auto& line : m_lines) {
        line.search(*this, m_search_text, m_search_results);
    }

    if (!m_search_results.empty()) {
        set_needs_display();
    }
}

void Document::clear_search() {
    clear_search_results();
    m_search_text = "";
    m_search_result_index = 0;
}

void Document::clear_search_results() {
    if (m_search_results.empty()) {
        return;
    }

    m_search_results.clear();
    set_needs_display();
}

void Document::set_search_text(String text) {
    if (m_search_text == text) {
        return;
    }

    clear_search();
    m_search_text = move(text);
    update_search_results();
}

void Document::move_cursor_to_next_search_match() {
    if (m_search_results.empty()) {
        return;
    }

    if (m_search_result_index >= m_search_results.size()) {
        m_search_result_index = 0;
        move_cursor_to_document_start();
    }

    while (m_search_results.range(m_search_result_index).ends_before(index_at_cursor())) {
        m_search_result_index++;
        if (m_search_result_index == m_search_results.size()) {
            m_search_result_index = 0;
            move_cursor_to_document_start();
        }
    }

    move_cursor_to(m_search_results.range(m_search_result_index).start());
    move_cursor_to(m_search_results.range(m_search_result_index).end(), MovementMode::Select);
    scroll_cursor_into_view();
    m_search_result_index++;
}

void Document::enter_interactive_search() {
    m_panel.enter_search(m_search_text);
    m_panel.send_status_message(String::format("Found %d result(s)", search_result_count()));
}

void Document::swap_lines_at_cursor(SwapDirection direction) {
    push_command<SwapLinesCommand>(direction);
}

void Document::select_word_at_cursor() {
    bool was_space = isspace(char_at_cursor());
    bool was_word = isword(char_at_cursor());
    while (index_into_line_at_cursor() > 0) {
        move_cursor_left(MovementMode::Move);
        if (isspace(char_at_cursor()) != was_space || isword(char_at_cursor()) != was_word) {
            move_cursor_right(MovementMode::Move);
            break;
        }
    }

    while (index_into_line_at_cursor() < line_at_cursor().length()) {
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

void Document::select_all() {
    int save_row_offset = m_row_offset;
    int save_col_offset = m_col_offset;

    move_cursor_to_document_start(MovementMode::Move);
    move_cursor_to_document_end(MovementMode::Select);

    scroll(save_row_offset - m_row_offset, save_col_offset - m_col_offset);
}

bool Document::notify_mouse_event(const App::MouseEvent& event) {
    bool handled = false;
    if (event.mouse_down() && event.left_button()) {
        move_cursor_to({ event.y(), event.x() }, MovementMode::Move);
        handled = true;
    } else if (event.mouse_double() && event.left_button()) {
        move_cursor_to({ event.y(), event.x() }, MovementMode::Move);
        select_word_at_cursor();
        handled = true;
    } else if (event.mouse_triple() && event.left_button()) {
        move_cursor_to({ event.y(), event.x() }, MovementMode::Move);
        select_line_at_cursor();
        handled = true;
    } else if (event.buttons_down() & App::MouseButton::Left) {
        move_cursor_to({ event.y(), event.x() }, MovementMode::Select);
        handled = true;
    } else if (event.mouse_scroll()) {
        scroll(2 * event.z(), 0);
        handled = true;
    }

    if (needs_display()) {
        display();
    } else {
        m_panel.notify_now_is_a_good_time_to_draw_cursor();
    }
    return handled;
}

void Document::finish_key_press() {
    scroll_cursor_into_view();

    if (preview_auto_complete()) {
        line_at_cursor().invalidate_rendered_contents();
        set_needs_display();
    }

    if (needs_display()) {
        display();
    } else {
        m_panel.notify_now_is_a_good_time_to_draw_cursor();
    }
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
            case 'D':
                delete_word(DeleteCharMode::Delete);
                break;
            default:
                break;
        }

        finish_key_press();
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
            case 'A':
                select_all();
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
                if (!input_text_mode()) {
                    set_show_line_numbers(!m_show_line_numbers);
                }
                break;
            case 'O':
                if (!input_text_mode()) {
                    m_panel.do_open_prompt();
                }
                break;
            case 'Q':
            case 'W':
                quit();
                break;
            case 'S':
                if (!input_text_mode()) {
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

        finish_key_press();
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
            if (!submittable() || &line_at_cursor() != &last_line()) {
                split_line_at_cursor();
            } else if (submittable() && on_submit) {
                on_submit();
            }
            break;
        case KeyPress::Key::Escape:
            clear_search();
            clear_selection(cursor());
            if (on_escape_press) {
                on_escape_press();
            }
            break;
        case '\t':
            if (m_auto_complete_mode == AutoCompleteMode::Always) {
                auto suggestions = m_panel.get_suggestions();
                if (suggestions.suggestion_count() == 1) {
                    auto suggestion = suggestions.suggestion_list()[0];
                    insert_text_at_cursor(
                        String(suggestion.string() + suggestions.suggestion_offset(), suggestion.size() - suggestions.suggestion_offset()));
                } else if (suggestions.suggestion_count() > 1) {
                    m_panel.handle_suggestions(suggestions);
                }
                break;
            }
            insert_char(press.key);
            break;
        default:
            if (isascii(press.key)) {
                insert_char(press.key);
            }
            break;
    }

    finish_key_press();
}
}
