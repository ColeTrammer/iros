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

SharedPtr<Document> Document::create_from_stdin(const String& path, Panel& panel) {
    auto file = Ext::File(stdin);
    file.set_should_close_file(false);

    Vector<Line> lines;
    auto result = file.read_all_lines(
        [&](auto line_string) -> bool {
            lines.add(Line(move(line_string)));
            return true;
        },
        Ext::StripTrailingNewlines::Yes);

    SharedPtr<Document> ret;

    if (!result) {
        panel.send_status_message(String::format("error reading stdin: `%s'", strerror(file.error())));
        ret = Document::create_empty();
        ret->set_name(path);
    } else {
        lines.add(Line(""));
        ret = make_shared<Document>(move(lines), path, InputMode::Document);
    }

    assert(freopen("/dev/tty", "r+", stdin));
    return ret;
}

SharedPtr<Document> Document::create_from_file(const String& path, Panel& panel) {
    auto file = Ext::File::create(path, "r");
    if (!file) {
        if (errno == ENOENT) {
            panel.send_status_message(String::format("new file: `%s'", path.string()));
            return make_shared<Document>(Vector<Line>(), path, InputMode::Document);
        }
        panel.send_status_message(String::format("error accessing file: `%s': `%s'", path.string(), strerror(errno)));
        return Document::create_empty();
    }

    Vector<Line> lines;
    auto result = file->read_all_lines(
        [&](auto line_string) -> bool {
            lines.add(Line(move(line_string)));
            return true;
        },
        Ext::StripTrailingNewlines::Yes);

    SharedPtr<Document> ret;

    if (!result) {
        panel.send_status_message(String::format("error reading file: `%s': `%s'", path.string(), strerror(file->error())));
        ret = Document::create_empty();
    } else {
        lines.add(Line(""));
        ret = make_shared<Document>(move(lines), path, InputMode::Document);
    }

    if (!file->close()) {
        panel.send_status_message(String::format("error closing file: `%s'", path.string()));
    }

    return ret;
}

SharedPtr<Document> Document::create_from_text(const String& text) {
    auto lines_view = text.split_view('\n');

    Vector<Line> lines(lines_view.size());
    for (auto& line_view : lines_view) {
        lines.add(Line(String(line_view)));
    }

    return make_shared<Document>(move(lines), "", InputMode::InputText);
}

SharedPtr<Document> Document::create_empty() {
    return make_shared<Document>(Vector<Line>(), "", InputMode::Document);
}

SharedPtr<Document> Document::create_single_line(String text) {
    Vector<Line> lines;
    lines.add(Line(move(text)));
    auto ret = make_shared<Document>(move(lines), "", InputMode::InputText);
    ret->set_submittable(true);
    ret->set_show_line_numbers(false);
    return ret;
}

Document::Document(Vector<Line> lines, String name, InputMode mode)
    : m_lines(move(lines)), m_name(move(name)), m_input_mode(mode), m_search_results(*this), m_syntax_highlighting_info(*this) {
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

size_t Document::cursor_index_in_content_string(const Cursor& cursor) const {
    if (input_text_mode() && num_lines() == 1) {
        return cursor.index_into_line();
    }

    size_t index = 0;
    for (int i = 0; i < cursor.line_index(); i++) {
        index += line_at_index(i).length() + 1;
    }

    index += cursor.index_into_line();
    return index;
}

void Document::set_needs_display() {
    for (auto* panel : m_panels) {
        panel->schedule_update();
    }
}

void Document::display(Panel& panel) const {
    auto& document = const_cast<Document&>(*this);

    int scroll_row_offset = panel.scroll_row_offset();
    int scroll_col_offset = panel.scroll_col_offset();

    auto render_index = text_index_at_absolute_position(panel, { scroll_row_offset, 0 });
    auto relative_start_position =
        line_at_index(render_index.line_index()).relative_position_of_index(*this, panel, render_index.index_into_line());
    render_index.set_index_into_line(0);

    auto selection_collection = panel.cursors().selections(*this);
    auto cursor_collection = panel.cursors().cursor_text_ranges(*this);
    DocumentTextRangeIterator metadata_iterator(render_index, m_syntax_highlighting_info, m_search_results, selection_collection,
                                                cursor_collection);

    int row = scroll_row_offset;
    for (; render_index.line_index() < num_lines() && row < scroll_row_offset + panel.rows();) {
        auto& line = line_at_index(render_index.line_index());
        row += line.render(document, panel, metadata_iterator, scroll_col_offset, relative_start_position.row, row - scroll_row_offset);
        render_index.set_line_index(render_index.line_index() + 1);
        relative_start_position = { 0, 0 };
    }
}

TextIndex Document::text_index_at_absolute_position(Panel& panel, const Position& position) const {
    if (position.row < 0) {
        return { 0, 0 };
    }

    int absolute_row = 0;
    for (auto& line : m_lines) {
        auto height_of_line = line.rendered_line_count(*this, panel);
        if (position.row < absolute_row + height_of_line) {
            return { index_of_line(line), line.index_of_relative_position(*this, panel, { position.row - absolute_row, position.col }) };
        }
        absolute_row += height_of_line;
    }

    return { num_lines() - 1, last_line().length() };
}

TextIndex Document::text_index_at_scrolled_position(Panel& panel, const Position& position) const {
    return text_index_at_absolute_position(panel, { position.row + panel.scroll_row_offset(), position.col + panel.scroll_col_offset() });
}

Position Document::relative_to_absolute_position(Panel& panel, const Line& reference_line, const Position& line_relative_position) const {
    return { reference_line.absolute_row_position(*this, panel) + line_relative_position.row, line_relative_position.col };
}

int Document::num_rendered_lines(Panel& panel) const {
    int total = 0;
    for (auto& line : m_lines) {
        total += line.rendered_line_count(*this, panel);
    }
    return total;
}

Position Document::cursor_position_on_panel(Panel& panel, Cursor& cursor) const {
    auto position = cursor.absolute_position(*this, panel);
    return { position.row - panel.scroll_row_offset(), position.col - panel.scroll_col_offset() };
}

int Document::index_of_line(const Line& line) const {
    return &line - m_lines.vector();
}

void Document::update_selection_state_for_mode(Cursor& cursor, MovementMode mode) {
    if (mode == MovementMode::Move) {
        clear_selection(cursor);
        return;
    }

    if (cursor.selection().empty()) {
        cursor.selection().begin(cursor.index());
    }
}

void Document::move_cursor_right_by_word(Panel& panel, Cursor& cursor, MovementMode mode) {
    move_cursor_right(panel, cursor, mode);

    auto& line = cursor.referenced_line(*this);
    while (cursor.index_into_line() < line.length() && !isword(cursor.referenced_character(*this))) {
        move_cursor_right(panel, cursor, mode);
    }

    while (cursor.index_into_line() < line.length() && isword(cursor.referenced_character(*this))) {
        move_cursor_right(panel, cursor, mode);
    }
}

void Document::move_cursor_left_by_word(Panel& panel, Cursor& cursor, MovementMode mode) {
    move_cursor_left(panel, cursor, mode);

    while (cursor.index_into_line() > 0 && !isword(cursor.referenced_character(*this))) {
        move_cursor_left(panel, cursor, mode);
    }

    bool found_word = false;
    while (cursor.index_into_line() > 0 && isword(cursor.referenced_character(*this))) {
        move_cursor_left(panel, cursor, mode);
        found_word = true;
    }

    if (found_word && !isword(cursor.referenced_character(*this))) {
        move_cursor_right(panel, cursor, mode);
    }
}

void Document::move_cursor_right(Panel& panel, Cursor& cursor, MovementMode mode) {
    auto& line = cursor.referenced_line(*this);
    int index_into_line = cursor.index_into_line();
    if (index_into_line == line.length()) {
        if (&line == &m_lines.last()) {
            return;
        }

        move_cursor_down(panel, cursor, mode);
        move_cursor_to_line_start(panel, cursor, mode);
        return;
    }

    auto& selection = cursor.selection();
    if (!selection.empty() && mode == MovementMode::Move) {
        auto selection_end = selection.normalized_end();
        clear_selection(cursor);
        move_cursor_to(panel, cursor, selection_end);
        return;
    }

    int new_index_into_line = cursor.index_into_line() + 1;
    if (mode == MovementMode::Select) {
        if (selection.empty()) {
            selection.begin(cursor.index());
        }
        selection.set_end_index_into_line(new_index_into_line);
        set_needs_display();
    }

    cursor.set_index_into_line(new_index_into_line);
    cursor.compute_max_col(*this, panel);
}

void Document::move_cursor_left(Panel& panel, Cursor& cursor, MovementMode mode) {
    auto& line = cursor.referenced_line(*this);
    int index_into_line = cursor.index_into_line();
    if (index_into_line == 0) {
        if (&line == &m_lines.first()) {
            return;
        }

        move_cursor_up(panel, cursor, mode);
        move_cursor_to_line_end(panel, cursor, mode);
        return;
    }

    auto& selection = cursor.selection();
    if (!selection.empty() && mode == MovementMode::Move) {
        auto selection_start = selection.normalized_start();
        clear_selection(cursor);
        move_cursor_to(panel, cursor, selection_start);
        return;
    }

    int new_index_into_line = index_into_line - 1;
    if (mode == MovementMode::Select) {
        if (selection.empty()) {
            selection.begin(cursor.index());
        }
        selection.set_end_index_into_line(new_index_into_line);
        set_needs_display();
    }

    cursor.set_index_into_line(new_index_into_line);
    cursor.compute_max_col(*this, panel);
}

void Document::move_cursor_down(Panel& panel, Cursor& cursor, MovementMode mode) {
    auto& prev_line = cursor.referenced_line(*this);
    if (&prev_line == &last_line() && last_line().relative_position_of_index(*this, panel, cursor.index_into_line()).row ==
                                          last_line().rendered_line_count(*this, panel) - 1) {
        move_cursor_to_line_end(panel, cursor, mode);
        return;
    }

    update_selection_state_for_mode(cursor, mode);

    auto prev_position = cursor.absolute_position(*this, panel);
    auto new_index = text_index_at_absolute_position(panel, { prev_position.row + 1, prev_position.col });

    cursor.set(new_index);

    clamp_cursor_to_line_end(panel, cursor);
    if (mode == MovementMode::Select) {
        cursor.selection().set_end(cursor.index());
        set_needs_display();
    }
}

void Document::move_cursor_up(Panel& panel, Cursor& cursor, MovementMode mode) {
    auto& prev_line = cursor.referenced_line(*this);
    if (&prev_line == &first_line() && first_line().relative_position_of_index(*this, panel, cursor.index_into_line()).row == 0) {
        move_cursor_to_line_start(panel, cursor, mode);
        return;
    }

    update_selection_state_for_mode(cursor, mode);

    auto prev_position = cursor.absolute_position(*this, panel);
    auto new_index = text_index_at_absolute_position(panel, { prev_position.row - 1, prev_position.col });

    cursor.set(new_index);

    clamp_cursor_to_line_end(panel, cursor);
    if (mode == MovementMode::Select) {
        cursor.selection().set_end(cursor.index());
        set_needs_display();
    }
}

void Document::clamp_cursor_to_line_end(Panel& panel, Cursor& cursor) {
    auto& line = cursor.referenced_line(*this);
    auto current_pos = cursor.relative_position(*this, panel);
    auto max_col = line.max_col_in_relative_row(*this, panel, current_pos.row);
    if (current_pos.col == max_col) {
        return;
    }

    if (current_pos.col > max_col) {
        cursor.set_index_into_line(line.index_of_relative_position(*this, panel, { current_pos.row, max_col }));
        return;
    }

    if (cursor.max_col() > current_pos.col) {
        cursor.set_index_into_line(line.index_of_relative_position(*this, panel, { current_pos.row, cursor.max_col() }));
        return;
    }
}

void Document::move_cursor_to_line_start(Panel& panel, Cursor& cursor, MovementMode mode) {
    update_selection_state_for_mode(cursor, mode);
    if (mode == MovementMode::Select) {
        cursor.selection().set_end_index_into_line(0);
        set_needs_display();
    }

    cursor.set_index_into_line(0);
    cursor.compute_max_col(*this, panel);
}

void Document::move_cursor_to_line_end(Panel& panel, Cursor& cursor, MovementMode mode) {
    auto& line = cursor.referenced_line(*this);

    update_selection_state_for_mode(cursor, mode);
    if (mode == MovementMode::Select) {
        cursor.selection().set_end_index_into_line(line.length());
        set_needs_display();
    }

    panel.set_scroll_col_offset(0);
    cursor.set_index_into_line(line.length());
    cursor.compute_max_col(*this, panel);
}

void Document::move_cursor_to_document_start(Panel& panel, Cursor& cursor, MovementMode mode) {
    move_cursor_to(panel, cursor, { 0, 0 }, mode);
}

void Document::move_cursor_to_document_end(Panel& panel, Cursor& cursor, MovementMode mode) {
    auto last_line_index = m_lines.size() - 1;
    auto& last_line = m_lines.last();
    move_cursor_to(panel, cursor, { last_line_index, last_line.length() }, mode);
}

void Document::scroll_cursor_into_view(Panel& panel, Cursor& cursor) {
    if (cursor_position_on_panel(panel, cursor).row < 0) {
        panel.scroll_up(-cursor_position_on_panel(panel, cursor).row);
    } else if (cursor_position_on_panel(panel, cursor).row >= panel.rows()) {
        panel.scroll_down(cursor_position_on_panel(panel, cursor).row - panel.rows() + 1);
    }

    if (cursor_position_on_panel(panel, cursor).col < 0) {
        panel.scroll_left(-cursor_position_on_panel(panel, cursor).col);
    } else if (cursor_position_on_panel(panel, cursor).col >= panel.cols()) {
        panel.scroll_right(cursor_position_on_panel(panel, cursor).col - panel.cols() + 1);
    }
}

void Document::move_cursor_page_up(Panel& panel, Cursor& cursor, MovementMode mode) {
    int rows_to_move = panel.rows() - 1;

    for (int i = 0; !cursor.at_document_start(*this) && i < rows_to_move; i++) {
        move_cursor_up(panel, cursor, mode);
    }
}

void Document::move_cursor_page_down(Panel& panel, Cursor& cursor, MovementMode mode) {
    int rows_to_move = panel.rows() - 1;

    for (int i = 0; !cursor.at_document_end(*this) <= num_lines() && i < rows_to_move; i++) {
        move_cursor_down(panel, cursor, mode);
    }
}

void Document::insert_char(Panel& panel, char c) {
    push_command<InsertCommand>(panel, String(c));
}

void Document::delete_char(Panel& panel, DeleteCharMode mode) {
    push_command<DeleteCommand>(panel, mode);
}

void Document::delete_word(Panel& panel, DeleteCharMode mode) {
    auto& cursors = panel.cursors();
    if (!cursors.main_cursor().selection().empty()) {
        delete_char(panel, mode);
        return;
    }

    for (auto& cursor : cursors) {
        int index_into_line = cursor.index_into_line();
        if ((mode == DeleteCharMode::Backspace && index_into_line == 0) ||
            (mode == DeleteCharMode::Delete && index_into_line == cursor.referenced_line(*this).length())) {
            continue;
        }

        if (mode == DeleteCharMode::Backspace) {
            move_cursor_left_by_word(panel, cursor, MovementMode::Select);
        } else {
            move_cursor_right_by_word(panel, cursor, MovementMode::Select);
        }

        swap_selection_start_and_cursor(panel, cursor);
    }
    push_command<DeleteCommand>(panel, mode);
}

void Document::swap_selection_start_and_cursor(Panel& panel, Cursor& cursor) {
    auto& selection = cursor.selection();
    auto start = selection.start();
    auto end = selection.end();

    move_cursor_to(panel, cursor, start);

    selection.set(start, end);
}

void Document::split_line_at_cursor(Panel& panel) {
    push_command<InsertCommand>(panel, "\n");
}

bool Document::execute_command(Panel& panel, Command& command) {
    return command.execute(panel);
}

void Document::redo(Panel& panel) {
    if (m_command_stack_index == m_command_stack.size()) {
        return;
    }

    auto& command = *m_command_stack[m_command_stack_index++];
    command.redo(panel);

    if (on_change) {
        on_change();
    }
}

void Document::undo(Panel& panel) {
    if (m_command_stack_index == 0) {
        return;
    }

    auto& command = *m_command_stack[--m_command_stack_index];
    command.undo(panel);
    update_search_results();
    update_syntax_highlighting();

    if (on_change) {
        on_change();
    }
}

Document::StateSnapshot Document::snapshot_state(Panel& panel) const {
    return { panel.cursors(), m_document_was_modified };
}

Document::Snapshot Document::snapshot(Panel& panel) const {
    return { Vector<Line>(m_lines), snapshot_state(panel) };
}

void Document::restore(MultiCursor& cursors, Snapshot s) {
    m_lines = move(s.lines);
    cursors = s.state.cursors;
    m_document_was_modified = s.state.document_was_modified;

    update_search_results();
    set_needs_display();
}

void Document::restore_state(MultiCursor& cursors, const StateSnapshot& s) {
    cursors = s.cursors;
    m_document_was_modified = s.document_was_modified;

    set_needs_display();
}

void Document::insert_text_at_cursor(Panel& panel, const String& text) {
    if (text.is_empty()) {
        return;
    }

    push_command<InsertCommand>(panel, text);
}

void Document::move_cursor_to(Panel& panel, Cursor& cursor, const TextIndex& index, MovementMode mode) {
    update_selection_state_for_mode(cursor, mode);
    while (cursor.line_index() < index.line_index()) {
        move_cursor_down(panel, cursor, mode);
    }
    while (cursor.line_index() > index.line_index()) {
        move_cursor_up(panel, cursor, mode);
    }

    while (cursor.index_into_line() < index.index_into_line()) {
        move_cursor_right(panel, cursor, mode);
    }
    while (cursor.index_into_line() > index.index_into_line()) {
        move_cursor_left(panel, cursor, mode);
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
    if (line_start == line_end) {
        for (int i = index_end - 1; i >= index_start; i--) {
            m_lines[line_start].remove_char_at(*this, i);
        }
    } else {
        auto split_start = m_lines[line_start].split_at(index_start);
        auto split_end = m_lines[line_end].split_at(index_end);
        for (int i = line_end - 1; i > line_start; i--) {
            remove_line(i);
        }

        m_lines[line_start].overwrite(*this, move(split_start.first), Line::OverwriteFrom::LineEnd);
        m_lines[line_start + 1].overwrite(*this, move(split_end.second), Line::OverwriteFrom::LineStart);

        merge_lines(line_start, line_start + 1);
    }
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
    did_delete_lines(index, 1);
}

void Document::insert_line(Line&& line, int index) {
    m_lines.insert(move(line), index);
    did_add_lines(index, 1);
}

void Document::merge_lines(int l1, int l2) {
    assert(l1 + 1 == l2);
    auto old_l1_length = m_lines[l1].length();
    m_lines[l1].combine_line(*this, m_lines[l2]);
    m_lines.remove(l2);
    did_merge_lines(l1, old_l1_length, l2);
}

void Document::split_line_at(const TextIndex& index) {
    auto& line = line_at_index(index.line_index());
    auto split_result = line.split_at(index.index_into_line());
    line.overwrite(*this, move(split_result.first), Line::OverwriteFrom::None);
    m_lines.insert(move(split_result.second), index.line_index() + 1);
    did_split_line(index.line_index(), index.index_into_line());
}

void Document::did_delete_lines(int line_index, int line_count) {
    for (auto* panel : m_panels) {
        panel->notify_did_delete_lines(line_index, line_count);
    }
}

void Document::did_add_lines(int line_index, int line_count) {
    for (auto* panel : m_panels) {
        panel->notify_did_add_lines(line_index, line_count);
    }
}

void Document::did_split_line(int line_index, int index_into_line) {
    for (auto* panel : m_panels) {
        panel->notify_did_split_line(line_index, index_into_line);
    }
}

void Document::did_merge_lines(int first_line_index, int first_line_length, int second_line_index) {
    for (auto* panel : m_panels) {
        panel->notify_did_merge_lines(first_line_index, first_line_length, second_line_index);
    }
}

void Document::did_add_to_line(int line_index, int index_into_line, int bytes_added) {
    for (auto* panel : m_panels) {
        panel->notify_did_add_to_line(line_index, index_into_line, bytes_added);
    }
}

void Document::did_delete_from_line(int line_index, int index_into_line, int bytes_deleted) {
    for (auto* panel : m_panels) {
        panel->notify_did_delete_from_line(line_index, index_into_line, bytes_deleted);
    }
}

void Document::register_panel(Panel& panel) {
    m_panels.add(&panel);
}

void Document::unregister_panel(Panel& panel) {
    m_panels.remove_element(&panel);
}

void Document::rotate_lines_up(int start, int end) {
    // Vector::rotate is exclusive, this is inclusive
    m_lines.rotate_left(start, end + 1);
}

void Document::rotate_lines_down(int start, int end) {
    // Vector::rotate is exclusive, this is inclusive
    m_lines.rotate_right(start, end + 1);
}

void Document::copy(Panel& panel, MultiCursor& cursors) {
    auto& cursor = cursors.main_cursor();
    if (cursor.selection().empty()) {
        String contents = cursor.referenced_line(*this).contents();
        if (!input_text_mode()) {
            contents += "\n";
        }
        panel.set_clipboard_contents(move(contents), true);
        return;
    }

    panel.set_clipboard_contents(selection_text(cursor));
}

void Document::cut(Panel& panel, MultiCursor& cursors) {
    auto& cursor = cursors.main_cursor();
    if (cursor.selection().empty()) {
        auto contents = cursor.referenced_line(*this).contents();
        if (!input_text_mode()) {
            contents += "\n";
        }
        panel.set_clipboard_contents(move(contents), true);
        push_command<DeleteLineCommand>(panel);
        return;
    }

    panel.set_clipboard_contents(selection_text(cursor));
    push_command<DeleteCommand>(panel, DeleteCharMode::Delete);
}

void Document::paste(Panel& panel, MultiCursor& cursors) {
    bool is_whole_line;
    auto text_to_insert = panel.clipboard_contents(is_whole_line);
    if (text_to_insert.is_empty()) {
        return;
    }

    auto& cursor = cursors.main_cursor();
    if (!input_text_mode() && cursor.selection().empty() && is_whole_line) {
        text_to_insert.remove_index(text_to_insert.size() - 1);
        push_command<InsertLineCommand>(panel, text_to_insert);
    } else {
        insert_text_at_cursor(panel, text_to_insert);
    }
}

void Document::set_show_line_numbers(bool b) {
    if (m_show_line_numbers != b) {
        m_show_line_numbers = b;
        for (auto* panel : m_panels) {
            panel->notify_line_count_changed();
        }
    }
}

void Document::set_word_wrap_enabled(bool b) {
    if (m_word_wrap_enabled == b) {
        return;
    }

    m_word_wrap_enabled = b;
    for (auto* panel : m_panels) {
        panel->set_scroll_col_offset(0);
    }
    invalidate_all_rendered_contents();
}

void Document::set_preview_auto_complete(bool b) {
    if (m_preview_auto_complete == b) {
        return;
    }

    m_preview_auto_complete = b;
    for (auto* panel : m_panels) {
        panel->cursors().main_cursor().referenced_line(*this).invalidate_rendered_contents(*this, *panel);
    }
}

void Document::invalidate_rendered_contents(const Line& line) {
    for (auto* panel : m_panels) {
        line.invalidate_rendered_contents(*this, *panel);
    }
}

void Document::invalidate_all_rendered_contents() {
    for (auto& line : m_lines) {
        invalidate_rendered_contents(line);
    }
    set_needs_display();
}

void Document::notify_panel_size_changed() {
    if (word_wrap_enabled()) {
        invalidate_all_rendered_contents();
    }
    set_needs_display();
}

void Document::go_to_line(Panel& panel) {
    auto maybe_result = panel.prompt("Go to line: ");
    if (!maybe_result.has_value()) {
        return;
    }

    auto& result = maybe_result.value();
    char* end_ptr = result.string();
    long line_number = strtol(result.string(), &end_ptr, 10);
    if (errno == ERANGE || end_ptr != result.string() + result.size() || line_number < 1 || line_number > num_lines()) {
        panel.send_status_message(String::format("Line `%s' is not between 1 and %d", result.string(), num_lines()));
        return;
    }

    auto& cursor = panel.cursors().main_cursor();

    clear_selection(cursor);
    cursor.set_line_index(line_number - 1);

    auto cursor_row_position = cursor.referenced_line(*this).absolute_row_position(*this, panel);

    int screen_midpoint = panel.rows() / 2;
    if (cursor_row_position < screen_midpoint) {
        panel.set_scroll_row_offset(0);
    } else {
        panel.set_scroll_row_offset(cursor_row_position - screen_midpoint);
    }

    move_cursor_to_line_start(panel, cursor);
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

void Document::save(Panel& panel) {
    if (m_name.is_empty()) {
        auto result = panel.prompt("Save as: ");
        if (!result.has_value()) {
            return;
        }

        if (access(result.value().string(), F_OK) == 0) {
            auto ok = panel.prompt(String::format("Are you sure you want to overwrite file `%s'? ", result.value().string()));
            if (!ok.has_value() || (ok.value() != "y" && ok.value() != "yes")) {
                return;
            }
        }

        m_name = move(result.value());
        guess_type_from_name();
    }

    if (access(m_name.string(), W_OK)) {
        if (errno != ENOENT) {
            panel.send_status_message(String::format("Permission to write file `%s' denied", m_name.string()));
            return;
        }
    }

    FILE* file = fopen(m_name.string(), "w");
    if (!file) {
        panel.send_status_message(String::format("Failed to save - `%s'", strerror(errno)));
        return;
    }

    if (m_lines.size() != 1 || !m_lines.first().empty()) {
        for (auto& line : m_lines) {
            fprintf(file, "%s\n", line.contents().string());
        }
    }

    if (ferror(file)) {
        panel.send_status_message(String::format("Failed to write to disk - `%s'", strerror(errno)));
        fclose(file);
        return;
    }

    if (fclose(file)) {
        panel.send_status_message(String::format("Failed to sync to disk - `%s'", strerror(errno)));
        return;
    }

    panel.send_status_message(String::format("Successfully saved file: `%s'", m_name.string()));
    m_document_was_modified = false;
}

void Document::quit(Panel& panel) {
    if (m_document_was_modified && !input_text_mode()) {
        auto result = panel.prompt("Quit without saving? ");
        if (!result.has_value() || (result.value() != "y" && result.value() != "yes")) {
            return;
        }
    }

    panel.quit();
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

void Document::move_cursor_to_next_search_match(Panel& panel, Cursor& cursor) {
    if (m_search_results.empty()) {
        return;
    }

    if (m_search_result_index >= m_search_results.size()) {
        m_search_result_index = 0;
        move_cursor_to_document_start(panel, cursor);
    }

    while (m_search_results.range(m_search_result_index).ends_before(cursor.index())) {
        m_search_result_index++;
        if (m_search_result_index == m_search_results.size()) {
            m_search_result_index = 0;
            move_cursor_to_document_start(panel, cursor);
        }
    }

    move_cursor_to(panel, cursor, m_search_results.range(m_search_result_index).start());
    move_cursor_to(panel, cursor, m_search_results.range(m_search_result_index).end(), MovementMode::Select);
    scroll_cursor_into_view(panel, cursor);
    m_search_result_index++;
}

void Document::enter_interactive_search(Panel& panel) {
    panel.enter_search(m_search_text);
    panel.send_status_message(String::format("Found %d result(s)", search_result_count()));
}

void Document::swap_lines_at_cursor(Panel& panel, SwapDirection direction) {
    push_command<SwapLinesCommand>(panel, direction);
}

void Document::select_next_word_at_cursor(Panel& panel) {
    auto& main_cursor = panel.cursors().main_cursor();
    if (main_cursor.selection().empty()) {
        select_word_at_cursor(panel, main_cursor);
        auto search_text = selection_text(main_cursor);
        set_search_text(move(search_text));

        // Set m_search_result_index to point just past the current cursor.
        while (m_search_result_index < m_search_results.size() &&
               m_search_results.range(m_search_result_index).ends_before(main_cursor.index())) {
            m_search_result_index++;
        }
        m_search_result_index %= m_search_results.size();
        return;
    }

    auto& result = m_search_results.range(m_search_result_index);
    panel.cursors().add_cursor_at(*this, panel, result.end(), { result.start(), result.end() });

    ++m_search_result_index;
    m_search_result_index %= m_search_results.size();
}

void Document::select_word_at_cursor(Panel& panel, Cursor& cursor) {
    bool was_space = isspace(cursor.referenced_character(*this));
    bool was_word = isword(cursor.referenced_character(*this));
    while (cursor.index_into_line() > 0) {
        move_cursor_left(panel, cursor, MovementMode::Move);
        if (isspace(cursor.referenced_character(*this)) != was_space || isword(cursor.referenced_character(*this)) != was_word) {
            move_cursor_right(panel, cursor, MovementMode::Move);
            break;
        }
    }

    while (cursor.index_into_line() < cursor.referenced_line(*this).length()) {
        move_cursor_right(panel, cursor, MovementMode::Select);
        if (isspace(cursor.referenced_character(*this)) != was_space || isword(cursor.referenced_character(*this)) != was_word) {
            break;
        }
    }
}

void Document::select_line_at_cursor(Panel& panel, Cursor& cursor) {
    move_cursor_to_line_start(panel, cursor, MovementMode::Move);
    move_cursor_down(panel, cursor, MovementMode::Select);
}

void Document::select_all(Panel& panel, Cursor& cursor) {
    move_cursor_to_document_start(panel, cursor, MovementMode::Move);
    move_cursor_to_document_end(panel, cursor, MovementMode::Select);
}

bool Document::notify_mouse_event(Panel& panel, const App::MouseEvent& event) {
    auto& cursors = panel.cursors();

    bool should_scroll_cursor_into_view = false;
    bool handled = false;
    if (event.mouse_down() && event.left_button()) {
        cursors.remove_secondary_cursors();
        auto& cursor = cursors.main_cursor();
        move_cursor_to(panel, cursor, { event.y(), event.x() }, MovementMode::Move);
        handled = true;
    } else if (event.mouse_double() && event.left_button()) {
        cursors.remove_secondary_cursors();
        auto& cursor = cursors.main_cursor();
        move_cursor_to(panel, cursor, { event.y(), event.x() }, MovementMode::Move);
        select_word_at_cursor(panel, cursor);
        handled = true;
    } else if (event.mouse_triple() && event.left_button()) {
        cursors.remove_secondary_cursors();
        auto& cursor = cursors.main_cursor();
        move_cursor_to(panel, cursor, { event.y(), event.x() }, MovementMode::Move);
        select_line_at_cursor(panel, cursor);
        handled = true;
    } else if (event.buttons_down() & App::MouseButton::Left) {
        cursors.remove_secondary_cursors();
        auto& cursor = cursors.main_cursor();
        move_cursor_to(panel, cursor, { event.y(), event.x() }, MovementMode::Select);
        handled = true;
    } else if (event.mouse_scroll()) {
        panel.scroll(2 * event.z(), 0);
        handled = true;
    }

    finish_input(panel, should_scroll_cursor_into_view);
    return handled;
}

void Document::finish_input(Panel& panel, bool should_scroll_cursor_into_view) {
    auto& cursors = panel.cursors();
    cursors.remove_duplicate_cursors();

    if (should_scroll_cursor_into_view) {
        scroll_cursor_into_view(panel, cursors.main_cursor());
    }

    if (preview_auto_complete()) {
        cursors.main_cursor().referenced_line(*this).invalidate_rendered_contents(*this, panel);
    }

    set_needs_display();
}

void Document::notify_key_pressed(Panel& panel, KeyPress press) {
    auto& cursors = panel.cursors();

    bool should_scroll_cursor_into_view = true;
    if (press.modifiers & KeyPress::Modifier::Alt) {
        switch (press.key) {
            case KeyPress::Key::DownArrow:
                swap_lines_at_cursor(panel, SwapDirection::Down);
                break;
            case KeyPress::Key::UpArrow:
                swap_lines_at_cursor(panel, SwapDirection::Up);
                break;
            case 'D':
                delete_word(panel, DeleteCharMode::Delete);
                break;
            default:
                break;
        }

        finish_input(panel, should_scroll_cursor_into_view);
        return;
    }

    if (press.modifiers & KeyPress::Modifier::Control) {
        switch (toupper(press.key)) {
            case KeyPress::Key::LeftArrow:
                for (auto& cursor : cursors) {
                    move_cursor_left_by_word(panel, cursor,
                                             press.modifiers & KeyPress::Modifier::Shift ? MovementMode::Select : MovementMode::Move);
                }
                break;
            case KeyPress::Key::RightArrow:
                for (auto& cursor : cursors) {
                    move_cursor_right_by_word(panel, cursor,
                                              press.modifiers & KeyPress::Modifier::Shift ? MovementMode::Select : MovementMode::Move);
                }
                break;
            case KeyPress::Key::DownArrow:
                if (press.modifiers & KeyPress::Modifier::Shift) {
                    cursors.add_cursor(*this, panel, AddCursorMode::Down);
                } else {
                    panel.scroll_down(1);
                    should_scroll_cursor_into_view = false;
                }
                break;
            case KeyPress::Key::UpArrow:
                if (press.modifiers & KeyPress::Modifier::Shift) {
                    cursors.add_cursor(*this, panel, AddCursorMode::Up);
                } else {
                    panel.scroll_up(1);
                    should_scroll_cursor_into_view = false;
                }
                break;
            case KeyPress::Key::Home:
                for (auto& cursor : cursors) {
                    move_cursor_to_document_start(panel, cursor,
                                                  press.modifiers & KeyPress::Modifier::Shift ? MovementMode::Select : MovementMode::Move);
                }
                break;
            case KeyPress::Key::End:
                for (auto& cursor : cursors) {
                    move_cursor_to_document_end(panel, cursor,
                                                press.modifiers & KeyPress::Modifier::Shift ? MovementMode::Select : MovementMode::Move);
                }
                break;
            case KeyPress::Key::Backspace:
                delete_word(panel, DeleteCharMode::Backspace);
                break;
            case KeyPress::Key::Delete:
                delete_word(panel, DeleteCharMode::Delete);
                break;
            case 'A':
                select_all(panel, cursors.main_cursor());
                should_scroll_cursor_into_view = false;
                break;
            case 'C':
                copy(panel, cursors);
                break;
            case 'D':
                select_next_word_at_cursor(panel);
                break;
            case 'F':
                enter_interactive_search(panel);
                break;
            case 'G':
                go_to_line(panel);
                break;
            case 'L':
                if (!input_text_mode()) {
                    set_show_line_numbers(!m_show_line_numbers);
                }
                break;
            case 'O':
                if (!input_text_mode()) {
                    panel.do_open_prompt();
                }
                break;
            case 'Q':
            case 'W':
                quit(panel);
                break;
            case 'S':
                if (!input_text_mode()) {
                    save(panel);
                }
                break;
            case 'V':
                paste(panel, cursors);
                break;
            case 'X':
                cut(panel, cursors);
                break;
            case 'Y':
                redo(panel);
                break;
            case 'Z':
                undo(panel);
                break;
            default:
                break;
        }

        finish_input(panel, should_scroll_cursor_into_view);
        return;
    }

    switch (press.key) {
        case KeyPress::Key::LeftArrow:
            for (auto& cursor : cursors) {
                move_cursor_left(panel, cursor, press.modifiers & KeyPress::Modifier::Shift ? MovementMode::Select : MovementMode::Move);
            }
            break;
        case KeyPress::Key::RightArrow:
            for (auto& cursor : cursors) {
                move_cursor_right(panel, cursor, press.modifiers & KeyPress::Modifier::Shift ? MovementMode::Select : MovementMode::Move);
            }
            break;
        case KeyPress::Key::DownArrow:
            for (auto& cursor : cursors) {
                move_cursor_down(panel, cursor, press.modifiers & KeyPress::Modifier::Shift ? MovementMode::Select : MovementMode::Move);
            }
            break;
        case KeyPress::Key::UpArrow:
            for (auto& cursor : cursors) {
                move_cursor_up(panel, cursor, press.modifiers & KeyPress::Modifier::Shift ? MovementMode::Select : MovementMode::Move);
            }
            break;
        case KeyPress::Key::Home:
            for (auto& cursor : cursors) {
                move_cursor_to_line_start(panel, cursor,
                                          press.modifiers & KeyPress::Modifier::Shift ? MovementMode::Select : MovementMode::Move);
            }
            break;
        case KeyPress::Key::End:
            for (auto& cursor : cursors) {
                move_cursor_to_line_end(panel, cursor,
                                        press.modifiers & KeyPress::Modifier::Shift ? MovementMode::Select : MovementMode::Move);
            }
            break;
        case KeyPress::Key::PageUp:
            for (auto& cursor : cursors) {
                move_cursor_page_up(panel, cursor, press.modifiers & KeyPress::Modifier::Shift ? MovementMode::Select : MovementMode::Move);
            }
            break;
        case KeyPress::Key::PageDown:
            for (auto& cursor : cursors) {
                move_cursor_page_down(panel, cursor,
                                      press.modifiers & KeyPress::Modifier::Shift ? MovementMode::Select : MovementMode::Move);
            }
            break;
        case KeyPress::Key::Backspace:
            delete_char(panel, DeleteCharMode::Backspace);
            break;
        case KeyPress::Key::Delete:
            delete_char(panel, DeleteCharMode::Delete);
            break;
        case KeyPress::Key::Enter:
            if (!submittable() || &cursors.main_cursor().referenced_line(*this) != &last_line()) {
                split_line_at_cursor(panel);
            } else if (submittable() && on_submit) {
                on_submit();
            }
            break;
        case KeyPress::Key::Escape:
            clear_search();
            cursors.remove_secondary_cursors();
            clear_selection(cursors.main_cursor());
            if (on_escape_press) {
                on_escape_press();
            }
            break;
        case '\t':
            if (m_auto_complete_mode == AutoCompleteMode::Always) {
                auto suggestions = panel.get_suggestions();
                if (suggestions.suggestion_count() == 1) {
                    auto suggestion = suggestions.suggestion_list()[0];
                    insert_text_at_cursor(panel, String(suggestion.string() + suggestions.suggestion_offset(),
                                                        suggestion.size() - suggestions.suggestion_offset()));
                } else if (suggestions.suggestion_count() > 1) {
                    panel.handle_suggestions(suggestions);
                }
                break;
            }
            insert_char(panel, press.key);
            break;
        default:
            if (isascii(press.key)) {
                insert_char(panel, press.key);
            }
            break;
    }

    finish_input(panel, should_scroll_cursor_into_view);
}
}
