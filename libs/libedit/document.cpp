#include <edit/command.h>
#include <edit/display.h>
#include <edit/document.h>
#include <edit/position.h>
#include <errno.h>
#include <eventloop/event.h>
#include <ext/file.h>
#include <graphics/point.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

namespace Edit {
static inline int isword(int c) {
    return isalnum(c) || c == '_';
}

SharedPtr<Document> Document::create_from_stdin(const String& path, Maybe<String>& error_message) {
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
        error_message = String::format("error reading stdin: `%s'", strerror(file.error()));
        ret = Document::create_empty();
        ret->set_name(path);
    } else {
        ret = make_shared<Document>(move(lines), path, InputMode::Document);
    }

    assert(freopen("/dev/tty", "r+", stdin));
    return ret;
}

SharedPtr<Document> Document::create_from_file(const String& path, Maybe<String>& error_message) {
    auto file = Ext::File::create(path, "r");
    if (!file) {
        if (errno == ENOENT) {
            error_message = String::format("new file: `%s'", path.string());
            return make_shared<Document>(Vector<Line>(), path, InputMode::Document);
        }
        error_message = String::format("error accessing file: `%s': `%s'", path.string(), strerror(errno));
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
        error_message = String::format("error reading file: `%s': `%s'", path.string(), strerror(file->error()));
        ret = Document::create_empty();
    } else {
        ret = make_shared<Document>(move(lines), path, InputMode::Document);
    }

    if (!file->close()) {
        error_message = String::format("error closing file: `%s'", path.string());
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
    for (auto* display : m_displays) {
        display->schedule_update();
    }
}

void Document::display(Display& display) const {
    auto& document = const_cast<Document&>(*this);

    int scroll_row_offset = display.scroll_row_offset();
    int scroll_col_offset = display.scroll_col_offset();

    auto render_index = text_index_at_absolute_position(display, { scroll_row_offset, 0 });
    auto relative_start_position =
        line_at_index(render_index.line_index()).relative_position_of_index(*this, display, render_index.index_into_line());
    render_index.set_index_into_line(0);

    auto selection_collection = display.cursors().selections(*this);
    auto cursor_collection = display.cursors().cursor_text_ranges(*this);
    DocumentTextRangeIterator metadata_iterator(render_index, m_syntax_highlighting_info, m_search_results, selection_collection,
                                                cursor_collection);

    int row = scroll_row_offset;
    for (; render_index.line_index() < num_lines() && row < scroll_row_offset + display.rows();) {
        auto& line = line_at_index(render_index.line_index());
        row += line.render(document, display, metadata_iterator, scroll_col_offset, relative_start_position.row, row - scroll_row_offset);
        render_index.set_line_index(render_index.line_index() + 1);
        relative_start_position = { 0, 0 };
    }
}

TextIndex Document::text_index_at_absolute_position(Display& display, const Position& position) const {
    if (position.row < 0) {
        return { 0, 0 };
    }

    int absolute_row = 0;
    for (auto& line : m_lines) {
        auto height_of_line = line.rendered_line_count(*this, display);
        if (position.row < absolute_row + height_of_line) {
            return { index_of_line(line), line.index_of_relative_position(*this, display, { position.row - absolute_row, position.col }) };
        }
        absolute_row += height_of_line;
    }

    return { num_lines() - 1, last_line().length() };
}

TextIndex Document::text_index_at_scrolled_position(Display& display, const Position& position) const {
    return text_index_at_absolute_position(display,
                                           { position.row + display.scroll_row_offset(), position.col + display.scroll_col_offset() });
}

Position Document::relative_to_absolute_position(Display& display, const Line& reference_line,
                                                 const Position& line_relative_position) const {
    return { reference_line.absolute_row_position(*this, display) + line_relative_position.row, line_relative_position.col };
}

int Document::num_rendered_lines(Display& display) const {
    int total = 0;
    for (auto& line : m_lines) {
        total += line.rendered_line_count(*this, display);
    }
    return total;
}

Position Document::cursor_position_on_display(Display& display, Cursor& cursor) const {
    auto position = cursor.absolute_position(*this, display);
    return { position.row - display.scroll_row_offset(), position.col - display.scroll_col_offset() };
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

void Document::move_cursor_right_by_word(Display& display, Cursor& cursor, MovementMode mode) {
    move_cursor_right(display, cursor, mode);

    auto& line = cursor.referenced_line(*this);
    while (cursor.index_into_line() < line.length() && !isword(cursor.referenced_character(*this))) {
        move_cursor_right(display, cursor, mode);
    }

    while (cursor.index_into_line() < line.length() && isword(cursor.referenced_character(*this))) {
        move_cursor_right(display, cursor, mode);
    }
}

void Document::move_cursor_left_by_word(Display& display, Cursor& cursor, MovementMode mode) {
    move_cursor_left(display, cursor, mode);

    while (cursor.index_into_line() > 0 && !isword(cursor.referenced_character(*this))) {
        move_cursor_left(display, cursor, mode);
    }

    bool found_word = false;
    while (cursor.index_into_line() > 0 && isword(cursor.referenced_character(*this))) {
        move_cursor_left(display, cursor, mode);
        found_word = true;
    }

    if (found_word && !isword(cursor.referenced_character(*this))) {
        move_cursor_right(display, cursor, mode);
    }
}

void Document::move_cursor_right(Display& display, Cursor& cursor, MovementMode mode) {
    auto& selection = cursor.selection();
    if (!selection.empty() && mode == MovementMode::Move) {
        auto selection_end = selection.normalized_end();
        clear_selection(cursor);
        move_cursor_to(display, cursor, selection_end);
        return;
    }

    auto& line = cursor.referenced_line(*this);
    int index_into_line = cursor.index_into_line();
    if (index_into_line == line.length()) {
        if (&line == &m_lines.last()) {
            return;
        }

        move_cursor_down(display, cursor, mode);
        move_cursor_to_line_start(display, cursor, mode);
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
    cursor.compute_max_col(*this, display);
}

void Document::move_cursor_left(Display& display, Cursor& cursor, MovementMode mode) {
    auto& selection = cursor.selection();
    if (!selection.empty() && mode == MovementMode::Move) {
        auto selection_start = selection.normalized_start();
        clear_selection(cursor);
        move_cursor_to(display, cursor, selection_start);
        return;
    }

    auto& line = cursor.referenced_line(*this);
    int index_into_line = cursor.index_into_line();
    if (index_into_line == 0) {
        if (&line == &m_lines.first()) {
            return;
        }

        move_cursor_up(display, cursor, mode);
        move_cursor_to_line_end(display, cursor, mode);
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
    cursor.compute_max_col(*this, display);
}

void Document::move_cursor_down(Display& display, Cursor& cursor, MovementMode mode) {
    auto& prev_line = cursor.referenced_line(*this);
    if (&prev_line == &last_line() && last_line().relative_position_of_index(*this, display, cursor.index_into_line()).row ==
                                          last_line().rendered_line_count(*this, display) - 1) {
        move_cursor_to_line_end(display, cursor, mode);
        return;
    }

    update_selection_state_for_mode(cursor, mode);

    auto prev_position = cursor.absolute_position(*this, display);
    auto new_index = text_index_at_absolute_position(display, { prev_position.row + 1, prev_position.col });

    cursor.set(new_index);

    clamp_cursor_to_line_end(display, cursor);
    if (mode == MovementMode::Select) {
        cursor.selection().set_end(cursor.index());
        set_needs_display();
    }
}

void Document::move_cursor_up(Display& display, Cursor& cursor, MovementMode mode) {
    auto& prev_line = cursor.referenced_line(*this);
    if (&prev_line == &first_line() && first_line().relative_position_of_index(*this, display, cursor.index_into_line()).row == 0) {
        move_cursor_to_line_start(display, cursor, mode);
        return;
    }

    update_selection_state_for_mode(cursor, mode);

    auto prev_position = cursor.absolute_position(*this, display);
    auto new_index = text_index_at_absolute_position(display, { prev_position.row - 1, prev_position.col });

    cursor.set(new_index);

    clamp_cursor_to_line_end(display, cursor);
    if (mode == MovementMode::Select) {
        cursor.selection().set_end(cursor.index());
        set_needs_display();
    }
}

void Document::clamp_cursor_to_line_end(Display& display, Cursor& cursor) {
    auto& line = cursor.referenced_line(*this);
    auto current_pos = cursor.relative_position(*this, display);
    auto max_col = line.max_col_in_relative_row(*this, display, current_pos.row);
    if (current_pos.col == max_col) {
        return;
    }

    if (current_pos.col > max_col) {
        cursor.set_index_into_line(line.index_of_relative_position(*this, display, { current_pos.row, max_col }));
        return;
    }

    if (cursor.max_col() > current_pos.col) {
        cursor.set_index_into_line(line.index_of_relative_position(*this, display, { current_pos.row, cursor.max_col() }));
        return;
    }
}

void Document::move_cursor_to_line_start(Display& display, Cursor& cursor, MovementMode mode) {
    update_selection_state_for_mode(cursor, mode);
    if (mode == MovementMode::Select) {
        cursor.selection().set_end_index_into_line(0);
        set_needs_display();
    }

    cursor.set_index_into_line(0);
    cursor.compute_max_col(*this, display);
}

void Document::move_cursor_to_line_end(Display& display, Cursor& cursor, MovementMode mode) {
    auto& line = cursor.referenced_line(*this);

    update_selection_state_for_mode(cursor, mode);
    if (mode == MovementMode::Select) {
        cursor.selection().set_end_index_into_line(line.length());
        set_needs_display();
    }

    display.set_scroll_col_offset(0);
    cursor.set_index_into_line(line.length());
    cursor.compute_max_col(*this, display);
}

void Document::move_cursor_to_document_start(Display& display, Cursor& cursor, MovementMode mode) {
    move_cursor_to(display, cursor, { 0, 0 }, mode);
}

void Document::move_cursor_to_document_end(Display& display, Cursor& cursor, MovementMode mode) {
    auto last_line_index = m_lines.size() - 1;
    auto& last_line = m_lines.last();
    move_cursor_to(display, cursor, { last_line_index, last_line.length() }, mode);
}

void Document::scroll_cursor_into_view(Display& display, Cursor& cursor) {
    if (cursor_position_on_display(display, cursor).row < 0) {
        display.scroll_up(-cursor_position_on_display(display, cursor).row);
    } else if (cursor_position_on_display(display, cursor).row >= display.rows()) {
        display.scroll_down(cursor_position_on_display(display, cursor).row - display.rows() + 1);
    }

    if (cursor_position_on_display(display, cursor).col < 0) {
        display.scroll_left(-cursor_position_on_display(display, cursor).col);
    } else if (cursor_position_on_display(display, cursor).col >= display.cols()) {
        display.scroll_right(cursor_position_on_display(display, cursor).col - display.cols() + 1);
    }
}

void Document::move_cursor_page_up(Display& display, Cursor& cursor, MovementMode mode) {
    int rows_to_move = display.rows() - 1;

    for (int i = 0; !cursor.at_document_start(*this) && i < rows_to_move; i++) {
        move_cursor_up(display, cursor, mode);
    }
}

void Document::move_cursor_page_down(Display& display, Cursor& cursor, MovementMode mode) {
    int rows_to_move = display.rows() - 1;

    for (int i = 0; !cursor.at_document_end(*this) <= num_lines() && i < rows_to_move; i++) {
        move_cursor_down(display, cursor, mode);
    }
}

void Document::insert_char(Display& display, char c) {
    push_command<InsertCommand>(display, String(c));
}

void Document::delete_char(Display& display, DeleteCharMode mode) {
    push_command<DeleteCommand>(display, mode);
}

void Document::delete_word(Display& display, DeleteCharMode mode) {
    auto& cursors = display.cursors();
    if (!cursors.main_cursor().selection().empty()) {
        delete_char(display, mode);
        return;
    }

    for (auto& cursor : cursors) {
        int index_into_line = cursor.index_into_line();
        if ((mode == DeleteCharMode::Backspace && index_into_line == 0) ||
            (mode == DeleteCharMode::Delete && index_into_line == cursor.referenced_line(*this).length())) {
            continue;
        }

        if (mode == DeleteCharMode::Backspace) {
            move_cursor_left_by_word(display, cursor, MovementMode::Select);
        } else {
            move_cursor_right_by_word(display, cursor, MovementMode::Select);
        }

        swap_selection_start_and_cursor(display, cursor);
    }
    push_command<DeleteCommand>(display, mode);
}

void Document::swap_selection_start_and_cursor(Display& display, Cursor& cursor) {
    auto& selection = cursor.selection();
    auto start = selection.start();
    auto end = selection.end();

    move_cursor_to(display, cursor, start);

    selection.set(start, end);
}

void Document::split_line_at_cursor(Display& display) {
    push_command<InsertCommand>(display, "\n");
}

bool Document::execute_command(Display& display, Command& command) {
    return command.execute(display);
}

void Document::redo(Display& display) {
    if (m_command_stack_index == m_command_stack.size()) {
        return;
    }

    auto& command = *m_command_stack[m_command_stack_index++];
    command.redo(display);

    if (on_change) {
        on_change();
    }
}

void Document::undo(Display& display) {
    if (m_command_stack_index == 0) {
        return;
    }

    auto& command = *m_command_stack[--m_command_stack_index];
    command.undo(display);
    update_search_results();
    update_syntax_highlighting();
    update_suggestions(display);

    if (on_change) {
        on_change();
    }
}

Document::StateSnapshot Document::snapshot_state(Display& display) const {
    return { display.cursors(), m_document_was_modified };
}

Document::Snapshot Document::snapshot(Display& display) const {
    return { Vector<Line>(m_lines), snapshot_state(display) };
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

void Document::insert_text_at_cursor(Display& display, const String& text) {
    if (text.empty()) {
        return;
    }

    push_command<InsertCommand>(display, text);
}

void Document::move_cursor_to(Display& display, Cursor& cursor, const TextIndex& index, MovementMode mode) {
    update_selection_state_for_mode(cursor, mode);
    while (cursor.line_index() < index.line_index()) {
        move_cursor_down(display, cursor, mode);
    }
    while (cursor.line_index() > index.line_index()) {
        move_cursor_up(display, cursor, mode);
    }

    while (cursor.index_into_line() < index.index_into_line()) {
        move_cursor_right(display, cursor, mode);
    }
    while (cursor.index_into_line() > index.index_into_line()) {
        move_cursor_left(display, cursor, mode);
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
    for (auto* display : m_displays) {
        display->notify_did_delete_lines(line_index, line_count);
    }
}

void Document::did_add_lines(int line_index, int line_count) {
    for (auto* display : m_displays) {
        display->notify_did_add_lines(line_index, line_count);
    }
}

void Document::did_split_line(int line_index, int index_into_line) {
    for (auto* display : m_displays) {
        display->notify_did_split_line(line_index, index_into_line);
    }
}

void Document::did_merge_lines(int first_line_index, int first_line_length, int second_line_index) {
    for (auto* display : m_displays) {
        display->notify_did_merge_lines(first_line_index, first_line_length, second_line_index);
    }
}

void Document::did_add_to_line(int line_index, int index_into_line, int bytes_added) {
    for (auto* display : m_displays) {
        display->notify_did_add_to_line(line_index, index_into_line, bytes_added);
    }
}

void Document::did_delete_from_line(int line_index, int index_into_line, int bytes_deleted) {
    for (auto* display : m_displays) {
        display->notify_did_delete_from_line(line_index, index_into_line, bytes_deleted);
    }
}

void Document::register_display(Display& display) {
    m_displays.add(&display);
}

void Document::unregister_display(Display& display) {
    m_displays.remove_element(&display);
}

void Document::rotate_lines_up(int start, int end) {
    // Vector::rotate is exclusive, this is inclusive
    m_lines.rotate_left(start, end + 1);
}

void Document::rotate_lines_down(int start, int end) {
    // Vector::rotate is exclusive, this is inclusive
    m_lines.rotate_right(start, end + 1);
}

void Document::copy(Display& display, MultiCursor& cursors) {
    auto& cursor = cursors.main_cursor();
    if (cursor.selection().empty()) {
        String contents = cursor.referenced_line(*this).contents();
        if (!input_text_mode()) {
            contents += "\n";
        }
        display.set_clipboard_contents(move(contents), true);
        return;
    }

    display.set_clipboard_contents(selection_text(cursor));
}

void Document::cut(Display& display, MultiCursor& cursors) {
    auto& cursor = cursors.main_cursor();
    if (cursor.selection().empty()) {
        auto contents = cursor.referenced_line(*this).contents();
        if (!input_text_mode()) {
            contents += "\n";
        }
        display.set_clipboard_contents(move(contents), true);
        push_command<DeleteLineCommand>(display);
        return;
    }

    display.set_clipboard_contents(selection_text(cursor));
    push_command<DeleteCommand>(display, DeleteCharMode::Delete);
}

void Document::paste(Display& display, MultiCursor& cursors) {
    bool is_whole_line;
    auto text_to_insert = display.clipboard_contents(is_whole_line);
    if (text_to_insert.empty()) {
        return;
    }

    auto& cursor = cursors.main_cursor();
    if (!input_text_mode() && cursor.selection().empty() && is_whole_line) {
        text_to_insert.remove_index(text_to_insert.size() - 1);
        push_command<InsertLineCommand>(display, text_to_insert);
    } else {
        insert_text_at_cursor(display, text_to_insert);
    }
}

void Document::set_show_line_numbers(bool b) {
    if (m_show_line_numbers != b) {
        m_show_line_numbers = b;
        for (auto* display : m_displays) {
            display->notify_line_count_changed();
        }
    }
}

void Document::set_word_wrap_enabled(bool b) {
    if (m_word_wrap_enabled == b) {
        return;
    }

    m_word_wrap_enabled = b;
    for (auto* display : m_displays) {
        display->set_scroll_col_offset(0);
    }
    invalidate_all_rendered_contents();
}

void Document::set_preview_auto_complete(bool b) {
    if (m_preview_auto_complete == b) {
        return;
    }

    m_preview_auto_complete = b;
    for (auto* display : m_displays) {
        display->cursors().main_cursor().referenced_line(*this).invalidate_rendered_contents(*this, *display);
    }
}

void Document::invalidate_rendered_contents(const Line& line) {
    for (auto* display : m_displays) {
        line.invalidate_rendered_contents(*this, *display);
    }
}

void Document::invalidate_all_rendered_contents() {
    for (auto& line : m_lines) {
        invalidate_rendered_contents(line);
    }
    set_needs_display();
}

void Document::notify_display_size_changed() {
    if (word_wrap_enabled()) {
        invalidate_all_rendered_contents();
    }
    set_needs_display();
}

void Document::go_to_line(Display& display) {
    auto maybe_result = display.prompt("Go to line: ");
    if (!maybe_result.has_value()) {
        return;
    }

    auto& result = maybe_result.value();
    char* end_ptr = result.string();
    long line_number = strtol(result.string(), &end_ptr, 10);
    if (errno == ERANGE || end_ptr != result.string() + result.size() || line_number < 1 || line_number > num_lines()) {
        display.send_status_message(String::format("Line `%s' is not between 1 and %d", result.string(), num_lines()));
        return;
    }

    auto& cursor = display.cursors().main_cursor();

    clear_selection(cursor);
    cursor.set_line_index(line_number - 1);

    auto cursor_row_position = cursor.referenced_line(*this).absolute_row_position(*this, display);

    int screen_midpoint = display.rows() / 2;
    if (cursor_row_position < screen_midpoint) {
        display.set_scroll_row_offset(0);
    } else {
        display.set_scroll_row_offset(cursor_row_position - screen_midpoint);
    }

    move_cursor_to_line_start(display, cursor);
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

void Document::update_suggestions(Display& display) {
    auto suggestions = display.get_suggestions();
    display.update_suggestions_panel(suggestions);
}

void Document::save(Display& display) {
    if (m_name.empty()) {
        auto result = display.prompt("Save as: ");
        if (!result.has_value()) {
            return;
        }

        if (access(result.value().string(), F_OK) == 0) {
            auto ok = display.prompt(String::format("Are you sure you want to overwrite file `%s'? ", result.value().string()));
            if (!ok.has_value() || (ok.value() != "y" && ok.value() != "yes")) {
                return;
            }
        }

        m_name = move(result.value());
        guess_type_from_name();
    }

    if (access(m_name.string(), W_OK)) {
        if (errno != ENOENT) {
            display.send_status_message(String::format("Permission to write file `%s' denied", m_name.string()));
            return;
        }
    }

    FILE* file = fopen(m_name.string(), "w");
    if (!file) {
        display.send_status_message(String::format("Failed to save - `%s'", strerror(errno)));
        return;
    }

    if (m_lines.size() != 1 || !m_lines.first().empty()) {
        for (auto& line : m_lines) {
            fprintf(file, "%s\n", line.contents().string());
        }
    }

    if (ferror(file)) {
        display.send_status_message(String::format("Failed to write to disk - `%s'", strerror(errno)));
        fclose(file);
        return;
    }

    if (fclose(file)) {
        display.send_status_message(String::format("Failed to sync to disk - `%s'", strerror(errno)));
        return;
    }

    display.send_status_message(String::format("Successfully saved file: `%s'", m_name.string()));
    m_document_was_modified = false;
}

void Document::quit(Display& display) {
    if (m_document_was_modified && !input_text_mode()) {
        auto result = display.prompt("Quit without saving? ");
        if (!result.has_value() || (result.value() != "y" && result.value() != "yes")) {
            return;
        }
    }

    display.quit();
}

void Document::update_syntax_highlighting() {
    highlight_document(*this);
}

void Document::update_search_results() {
    clear_search_results();
    if (m_search_text.empty()) {
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

void Document::move_cursor_to_next_search_match(Display& display, Cursor& cursor) {
    if (m_search_results.empty()) {
        return;
    }

    if (m_search_result_index >= m_search_results.size()) {
        m_search_result_index = 0;
        move_cursor_to_document_start(display, cursor);
    }

    while (m_search_results.range(m_search_result_index).ends_before(cursor.index())) {
        m_search_result_index++;
        if (m_search_result_index == m_search_results.size()) {
            m_search_result_index = 0;
            move_cursor_to_document_start(display, cursor);
        }
    }

    move_cursor_to(display, cursor, m_search_results.range(m_search_result_index).start());
    move_cursor_to(display, cursor, m_search_results.range(m_search_result_index).end(), MovementMode::Select);
    scroll_cursor_into_view(display, cursor);
    m_search_result_index++;
}

void Document::enter_interactive_search(Display& display) {
    display.enter_search(m_search_text);
    display.send_status_message(String::format("Found %d result(s)", search_result_count()));
}

void Document::swap_lines_at_cursor(Display& display, SwapDirection direction) {
    push_command<SwapLinesCommand>(display, direction);
}

void Document::select_next_word_at_cursor(Display& display) {
    auto& main_cursor = display.cursors().main_cursor();
    if (main_cursor.selection().empty()) {
        select_word_at_cursor(display, main_cursor);
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
    display.cursors().add_cursor_at(*this, display, result.end(), { result.start(), result.end() });

    ++m_search_result_index;
    m_search_result_index %= m_search_results.size();
}

void Document::select_word_at_cursor(Display& display, Cursor& cursor) {
    bool was_space = isspace(cursor.referenced_character(*this));
    bool was_word = isword(cursor.referenced_character(*this));
    while (cursor.index_into_line() > 0) {
        move_cursor_left(display, cursor, MovementMode::Move);
        if (isspace(cursor.referenced_character(*this)) != was_space || isword(cursor.referenced_character(*this)) != was_word) {
            move_cursor_right(display, cursor, MovementMode::Move);
            break;
        }
    }

    while (cursor.index_into_line() < cursor.referenced_line(*this).length()) {
        move_cursor_right(display, cursor, MovementMode::Select);
        if (isspace(cursor.referenced_character(*this)) != was_space || isword(cursor.referenced_character(*this)) != was_word) {
            break;
        }
    }
}

void Document::select_line_at_cursor(Display& display, Cursor& cursor) {
    move_cursor_to_line_start(display, cursor, MovementMode::Move);
    move_cursor_down(display, cursor, MovementMode::Select);
}

void Document::select_all(Display& display, Cursor& cursor) {
    move_cursor_to_document_start(display, cursor, MovementMode::Move);
    move_cursor_to_document_end(display, cursor, MovementMode::Select);
}

void Document::insert_suggestion(Display& display, const Suggestions& suggestions, int suggestion_index) {
    auto suggestion = suggestions.suggestion_list()[suggestion_index];
    insert_text_at_cursor(
        display, String(suggestion.string() + suggestions.suggestion_offset(), suggestion.size() - suggestions.suggestion_offset()));
}

bool Document::notify_mouse_event(Display& display, const App::MouseEvent& event) {
    auto& cursors = display.cursors();

    auto index = display.text_index_at_mouse_position({ event.x(), event.y() });

    bool should_scroll_cursor_into_view = false;
    bool handled = false;
    if (event.mouse_down() && event.left_button()) {
        cursors.remove_secondary_cursors();
        auto& cursor = cursors.main_cursor();
        move_cursor_to(display, cursor, index, MovementMode::Move);
        handled = true;
    } else if (event.mouse_double() && event.left_button()) {
        cursors.remove_secondary_cursors();
        auto& cursor = cursors.main_cursor();
        move_cursor_to(display, cursor, index, MovementMode::Move);
        select_word_at_cursor(display, cursor);
        handled = true;
    } else if (event.mouse_triple() && event.left_button()) {
        cursors.remove_secondary_cursors();
        auto& cursor = cursors.main_cursor();
        move_cursor_to(display, cursor, index, MovementMode::Move);
        select_line_at_cursor(display, cursor);
        handled = true;
    } else if (event.buttons_down() & App::MouseButton::Left) {
        cursors.remove_secondary_cursors();
        auto& cursor = cursors.main_cursor();
        move_cursor_to(display, cursor, index, MovementMode::Select);
        handled = true;
    } else if (event.mouse_scroll()) {
        display.scroll(2 * event.z(), 0);
        handled = true;
    }

    finish_input(display, should_scroll_cursor_into_view);
    return handled;
}

void Document::finish_input(Display& display, bool should_scroll_cursor_into_view) {
    auto& cursors = display.cursors();
    cursors.remove_duplicate_cursors();

    if (should_scroll_cursor_into_view) {
        scroll_cursor_into_view(display, cursors.main_cursor());
    }

    update_suggestions(display);
    if (preview_auto_complete()) {
        cursors.main_cursor().referenced_line(*this).invalidate_rendered_contents(*this, display);
    }

    set_needs_display();
}

void Document::notify_key_pressed(Display& display, const App::KeyEvent& event) {
    auto& cursors = display.cursors();

    bool should_scroll_cursor_into_view = true;
    if (event.alt_down()) {
        switch (event.key()) {
            case App::Key::DownArrow:
                swap_lines_at_cursor(display, SwapDirection::Down);
                break;
            case App::Key::UpArrow:
                swap_lines_at_cursor(display, SwapDirection::Up);
                break;
            case App::Key::D:
                delete_word(display, DeleteCharMode::Delete);
                break;
            default:
                break;
        }

        finish_input(display, should_scroll_cursor_into_view);
        return;
    }

    if (event.control_down()) {
        switch (event.key()) {
            case App::Key::LeftArrow:
                for (auto& cursor : cursors) {
                    move_cursor_left_by_word(display, cursor, event.shift_down() ? MovementMode::Select : MovementMode::Move);
                }
                break;
            case App::Key::RightArrow:
                for (auto& cursor : cursors) {
                    move_cursor_right_by_word(display, cursor, event.shift_down() ? MovementMode::Select : MovementMode::Move);
                }
                break;
            case App::Key::DownArrow:
                if (event.shift_down()) {
                    cursors.add_cursor(*this, display, AddCursorMode::Down);
                } else {
                    display.scroll_down(1);
                    should_scroll_cursor_into_view = false;
                }
                break;
            case App::Key::UpArrow:
                if (event.shift_down()) {
                    cursors.add_cursor(*this, display, AddCursorMode::Up);
                } else {
                    display.scroll_up(1);
                    should_scroll_cursor_into_view = false;
                }
                break;
            case App::Key::Home:
                for (auto& cursor : cursors) {
                    move_cursor_to_document_start(display, cursor, event.shift_down() ? MovementMode::Select : MovementMode::Move);
                }
                break;
            case App::Key::End:
                for (auto& cursor : cursors) {
                    move_cursor_to_document_end(display, cursor, event.shift_down() ? MovementMode::Select : MovementMode::Move);
                }
                break;
            case App::Key::Backspace:
                delete_word(display, DeleteCharMode::Backspace);
                break;
            case App::Key::Delete:
                delete_word(display, DeleteCharMode::Delete);
                break;
            case App::Key::A:
                select_all(display, cursors.main_cursor());
                should_scroll_cursor_into_view = false;
                break;
            case App::Key::C:
                copy(display, cursors);
                break;
            case App::Key::D:
                select_next_word_at_cursor(display);
                break;
            case App::Key::F:
                enter_interactive_search(display);
                break;
            case App::Key::G:
                go_to_line(display);
                break;
            case App::Key::L:
                if (!input_text_mode()) {
                    set_show_line_numbers(!m_show_line_numbers);
                }
                break;
            case App::Key::O:
                if (!input_text_mode()) {
                    display.do_open_prompt();
                }
                break;
            case App::Key::Q:
            case App::Key::W:
                quit(display);
                break;
            case App::Key::S:
                if (!input_text_mode()) {
                    save(display);
                }
                break;
            case App::Key::V:
                paste(display, cursors);
                break;
            case App::Key::X:
                cut(display, cursors);
                break;
            case App::Key::Y:
                redo(display);
                break;
            case App::Key::Z:
                undo(display);
                break;
            default:
                break;
        }

        finish_input(display, should_scroll_cursor_into_view);
        return;
    }

    switch (event.key()) {
        case App::Key::LeftArrow:
            for (auto& cursor : cursors) {
                move_cursor_left(display, cursor, event.shift_down() ? MovementMode::Select : MovementMode::Move);
            }
            break;
        case App::Key::RightArrow:
            for (auto& cursor : cursors) {
                move_cursor_right(display, cursor, event.shift_down() ? MovementMode::Select : MovementMode::Move);
            }
            break;
        case App::Key::DownArrow:
            for (auto& cursor : cursors) {
                move_cursor_down(display, cursor, event.shift_down() ? MovementMode::Select : MovementMode::Move);
            }
            break;
        case App::Key::UpArrow:
            for (auto& cursor : cursors) {
                move_cursor_up(display, cursor, event.shift_down() ? MovementMode::Select : MovementMode::Move);
            }
            break;
        case App::Key::Home:
            for (auto& cursor : cursors) {
                move_cursor_to_line_start(display, cursor, event.shift_down() ? MovementMode::Select : MovementMode::Move);
            }
            break;
        case App::Key::End:
            for (auto& cursor : cursors) {
                move_cursor_to_line_end(display, cursor, event.shift_down() ? MovementMode::Select : MovementMode::Move);
            }
            break;
        case App::Key::PageUp:
            for (auto& cursor : cursors) {
                move_cursor_page_up(display, cursor, event.shift_down() ? MovementMode::Select : MovementMode::Move);
            }
            break;
        case App::Key::PageDown:
            for (auto& cursor : cursors) {
                move_cursor_page_down(display, cursor, event.shift_down() ? MovementMode::Select : MovementMode::Move);
            }
            break;
        case App::Key::Backspace:
            delete_char(display, DeleteCharMode::Backspace);
            break;
        case App::Key::Delete:
            delete_char(display, DeleteCharMode::Delete);
            break;
        case App::Key::Enter:
            if (!submittable() || &cursors.main_cursor().referenced_line(*this) != &last_line()) {
                split_line_at_cursor(display);
            } else if (submittable() && on_submit) {
                on_submit();
            }
            break;
        case App::Key::Escape:
            clear_search();
            cursors.remove_secondary_cursors();
            clear_selection(cursors.main_cursor());
            if (on_escape_press) {
                on_escape_press();
            }
            break;
        case App::Key::Tab:
            if (m_auto_complete_mode == AutoCompleteMode::Always) {
                auto suggestions = display.get_suggestions();
                if (suggestions.suggestion_count() == 1) {
                    insert_suggestion(display, suggestions, 0);
                } else if (suggestions.suggestion_count() > 1) {
                    display.show_suggestions_panel();
                    display.update_suggestions_panel(suggestions);
                }
                break;
            }
            insert_char(display, '\t');
            break;
        default:
            insert_text_at_cursor(display, event.text());
            break;
    }

    finish_input(display, should_scroll_cursor_into_view);
}
}
