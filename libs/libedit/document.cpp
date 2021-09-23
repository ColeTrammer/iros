#include <edit/command.h>
#include <edit/display.h>
#include <edit/document.h>
#include <errno.h>
#include <eventloop/event.h>
#include <eventloop/widget_events.h>
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
        ret = Document::create(nullptr, move(lines), path, InputMode::Document);
    }

    assert(freopen("/dev/tty", "r+", stdin));
    return ret;
}

SharedPtr<Document> Document::create_from_file(const String& path, Maybe<String>& error_message) {
    auto file = Ext::File::create(path, "r");
    if (!file) {
        if (errno == ENOENT) {
            error_message = String::format("new file: `%s'", path.string());
            return Document::create(nullptr, Vector<Line>(), path, InputMode::Document);
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
        ret = Document::create(nullptr, move(lines), path, InputMode::Document);
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

    return Document::create(nullptr, move(lines), "", InputMode::InputText);
}

SharedPtr<Document> Document::create_empty() {
    return Document::create(nullptr, Vector<Line>(), "", InputMode::Document);
}

Document::Document(Vector<Line> lines, String name, InputMode mode) : m_lines(move(lines)), m_name(move(name)), m_input_mode(mode) {
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

    set_convert_tabs_to_spaces(other.m_convert_tabs_to_spaces);
}

String Document::content_string() const {
    if (input_text_mode() && line_count() == 1) {
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
    if (input_text_mode() && line_count() == 1) {
        return cursor.index_into_line();
    }

    size_t index = 0;
    for (int i = 0; i < cursor.line_index(); i++) {
        index += line_at_index(i).length() + 1;
    }

    index += cursor.index_into_line();
    return index;
}

int Document::num_rendered_lines(Display& display) const {
    if (!display.word_wrap_enabled()) {
        return line_count();
    }

    int total = 0;
    for (int i = 0; i < line_count(); i++) {
        total += display.rendered_line_count(i);
    }
    return total;
}

void Document::update_selection_state_for_mode(Cursor& cursor, MovementMode mode) {
    if (mode == MovementMode::Move) {
        cursor.clear_selection();
        return;
    }

    if (cursor.selection().empty()) {
        cursor.set_selection_anchor(cursor.index());
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
    auto selection = cursor.selection();
    if (!selection.empty() && mode == MovementMode::Move) {
        auto selection_end = selection.end();
        cursor.clear_selection();
        move_cursor_to(display, cursor, selection_end);
        return;
    }

    if (cursor.at_line_end(*this)) {
        if (cursor.at_last_line(*this)) {
            return;
        }

        move_cursor_down(display, cursor, mode);
        move_cursor_to_line_start(display, cursor, mode);
        return;
    }

    update_selection_state_for_mode(cursor, mode);

    cursor.set(display.next_index_into_line(cursor.index()));
    cursor.compute_max_col(display);
}

void Document::move_cursor_left(Display& display, Cursor& cursor, MovementMode mode) {
    auto selection = cursor.selection();
    if (!selection.empty() && mode == MovementMode::Move) {
        auto selection_start = selection.start();
        cursor.clear_selection();
        move_cursor_to(display, cursor, selection_start);
        return;
    }

    if (cursor.at_line_start(*this)) {
        if (cursor.at_first_line(*this)) {
            return;
        }

        move_cursor_up(display, cursor, mode);
        move_cursor_to_line_end(display, cursor, mode);
        return;
    }

    update_selection_state_for_mode(cursor, mode);

    cursor.set(display.prev_index_into_line(cursor.index()));
    cursor.compute_max_col(display);
}

void Document::move_cursor_down(Display& display, Cursor& cursor, MovementMode mode) {
    auto prev_position = cursor.relative_position(display);
    if (cursor.at_last_line(*this) && prev_position.row() == display.rendered_line_count(last_line_index()) - 1) {
        move_cursor_to_line_end(display, cursor, mode);
        return;
    }

    update_selection_state_for_mode(cursor, mode);

    auto new_index = [&] {
        if (prev_position.row() == display.rendered_line_count(cursor.line_index()) - 1) {
            return display.text_index_at_absolute_position({ cursor.line_index() + 1, 0, prev_position.col() });
        }
        return display.text_index_at_absolute_position({ cursor.line_index(), prev_position.row() + 1, prev_position.col() });
    }();

    cursor.set(new_index);

    move_cursor_to_max_col_position(display, cursor);
}

void Document::move_cursor_up(Display& display, Cursor& cursor, MovementMode mode) {
    auto prev_position = cursor.relative_position(display);
    if (cursor.at_first_line(*this) && prev_position.row() == 0) {
        move_cursor_to_line_start(display, cursor, mode);
        return;
    }

    update_selection_state_for_mode(cursor, mode);

    auto new_index = [&] {
        if (prev_position.row() == 0) {
            return display.text_index_at_absolute_position(
                { cursor.line_index() - 1, display.rendered_line_count(cursor.line_index() - 1) - 1, prev_position.col() });
        }
        return display.text_index_at_absolute_position({ cursor.line_index(), prev_position.row() - 1, prev_position.col() });
    }();

    cursor.set(new_index);

    move_cursor_to_max_col_position(display, cursor);
}

void Document::move_cursor_to_max_col_position(Display& display, Cursor& cursor) {
    auto current_pos = cursor.relative_position(display);
    if (cursor.max_col() > current_pos.col()) {
        cursor.set(display.text_index_at_absolute_position({ cursor.line_index(), current_pos.row(), cursor.max_col() }));
    }
}

void Document::move_cursor_to_line_start(Display& display, Cursor& cursor, MovementMode mode) {
    update_selection_state_for_mode(cursor, mode);

    cursor.set_index_into_line(0);
    cursor.compute_max_col(display);
}

void Document::move_cursor_to_line_end(Display& display, Cursor& cursor, MovementMode mode) {
    update_selection_state_for_mode(cursor, mode);

    display.set_scroll_offset({ display.scroll_offset().line_index(), display.scroll_offset().relative_row(), 0 });

    auto& line = cursor.referenced_line(*this);
    cursor.set_index_into_line(line.length());
    cursor.compute_max_col(display);
}

void Document::move_cursor_to_document_start(Display& display, Cursor& cursor, MovementMode mode) {
    move_cursor_to(display, cursor, { 0, 0 }, mode);
}

void Document::move_cursor_to_document_end(Display& display, Cursor& cursor, MovementMode mode) {
    auto last_line_index = m_lines.size() - 1;
    auto& last_line = m_lines.last();
    move_cursor_to(display, cursor, { last_line_index, last_line.length() }, mode);
}

void Document::move_cursor_page_up(Display& display, Cursor& cursor, MovementMode mode) {
    int rows_to_move = display.rows() - 1;

    for (int i = 0; !cursor.at_document_start(*this) && i < rows_to_move; i++) {
        move_cursor_up(display, cursor, mode);
    }
}

void Document::move_cursor_page_down(Display& display, Cursor& cursor, MovementMode mode) {
    int rows_to_move = display.rows() - 1;

    for (int i = 0; !cursor.at_document_end(*this) <= line_count() && i < rows_to_move; i++) {
        move_cursor_down(display, cursor, mode);
    }
}

void Document::insert_char(Display& display, char c) {
    push_command<InsertCommand>(display, String(c));
}

void Document::delete_line(Display& display) {
    auto saved_max_col = Vector<int> {};

    // NOTE: subsequent cursors on the same line will be removed, so their max col can't be saved.
    auto last_line_index_seen = -1;
    for (auto& cursor : display.cursors()) {
        if (cursor.line_index() != last_line_index_seen) {
            saved_max_col.add(cursor.max_col());
        }
        last_line_index_seen = cursor.line_index();
    }

    auto group = make_unique<CommandGroup>(*this, "DeleteLine"sv);
    group->add<MovementCommand>(*this, [&](Display& display, MultiCursor& cursors) {
        for (auto& cursor : cursors) {
            select_line_at_cursor(display, cursor);
        }
        cursors.remove_duplicate_cursors();
    });
    group->add<DeleteCommand>(*this);
    group->add<MovementCommand>(*this, [&](Display& display, MultiCursor& cursors) {
        for (int i = 0; i < cursors.size(); i++) {
            auto& cursor = cursors[i];
            cursor.set_max_col(saved_max_col[i]);
            move_cursor_to_max_col_position(display, cursor);
        }
    });

    push_command(display, move(group));
}

void Document::delete_char(Display& display, DeleteCharMode mode) {
    auto group = make_unique<CommandGroup>(*this, mode == DeleteCharMode::Backspace ? "DeleteCharLeft" : "DeleteCharRight",
                                           CommandGroup::ShouldMerge::Yes);
    group->add<MovementCommand>(*this, [this, mode](Display& display, MultiCursor& cursors) {
        for (auto& cursor : cursors) {
            if (cursor.selection().empty()) {
                if (mode == DeleteCharMode::Backspace) {
                    move_cursor_left(display, cursor, MovementMode::Select);
                } else {
                    move_cursor_right(display, cursor, MovementMode::Select);
                }
                swap_selection_anchor_and_cursor(display, cursor);
            }
        }
    });
    group->add<DeleteCommand>(*this);

    push_command(display, move(group));
}

void Document::delete_word(Display& display, DeleteCharMode mode) {
    auto group = make_unique<CommandGroup>(*this, "DeleteWord");
    group->add<MovementCommand>(*this, [this, mode](Display& display, MultiCursor& cursors) {
        for (auto& cursor : cursors) {
            if (!cursor.selection().empty()) {
                continue;
            }

            if (mode == DeleteCharMode::Backspace) {
                move_cursor_left_by_word(display, cursor, MovementMode::Select);
            } else {
                move_cursor_right_by_word(display, cursor, MovementMode::Select);
            }

            swap_selection_anchor_and_cursor(display, cursor);
        }
    });
    group->add<DeleteCommand>(*this);

    push_command(display, move(group));
}

void Document::swap_selection_anchor_and_cursor(Display& display, Cursor& cursor) {
    auto index = cursor.index();
    auto selection_start = cursor.selection_anchor();

    move_cursor_to(display, cursor, selection_start);
    cursor.set_selection_anchor(index);
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
    m_document_was_modified = true;
    update_syntax_highlighting();
    update_suggestions(display);

    emit<Change>();
}

void Document::undo(Display& display) {
    if (m_command_stack_index == 0) {
        return;
    }

    auto& command = *m_command_stack[--m_command_stack_index];
    command.undo(display);
    update_syntax_highlighting();
    update_suggestions(display);

    emit<Change>();
}

Document::StateSnapshot Document::snapshot_state(Display& display) const {
    return { display.cursors().snapshot(), m_document_was_modified };
}

Document::Snapshot Document::snapshot(Display& display) const {
    return { Vector<Line>(m_lines), snapshot_state(display) };
}

void Document::restore(MultiCursor& cursors, Snapshot s) {
    m_lines = move(s.lines);
    restore_state(cursors, s.state);
}

void Document::restore_state(MultiCursor& cursors, const StateSnapshot& s) {
    cursors.restore(*this, s.cursors);
    m_document_was_modified = s.document_was_modified;
}

void Document::insert_line_at_cursor(Display& display, const String& line_text) {
    auto saved_indices_into_line = Vector<int> {};
    for (auto& cursor : display.cursors()) {
        saved_indices_into_line.add(cursor.index_into_line());
    }

    auto group = make_unique<CommandGroup>(*this, "InsertLine");
    group->add<MovementCommand>(*this, [&](Display& display, MultiCursor& cursors) {
        for (auto& cursor : cursors) {
            move_cursor_to_line_start(display, cursor);
        }
    });
    group->add<InsertCommand>(*this, line_text);
    group->add<MovementCommand>(*this, [&](Display&, MultiCursor& cursors) {
        for (int i = 0; i < cursors.size(); i++) {
            auto& cursor = cursors[i];
            cursor.set_index_into_line(saved_indices_into_line[i]);
        }
    });

    push_command(display, move(group));
}

void Document::insert_text_at_cursor(Display& display, const String& text) {
    auto all_cursors_have_selection = [&] {
        for (auto& cursor : display.cursors()) {
            if (cursor.selection().empty()) {
                return false;
            }
        }
        return true;
    }();

    if (all_cursors_have_selection) {
        struct InsertAround {
            String left;
            String right;
        };
        auto insert_around_result = [&]() -> Maybe<InsertAround> {
            if (text.view() == "<") {
                return InsertAround { "<", ">" };
            }
            if (text.view() == "'") {
                return InsertAround { "'", "'" };
            }
            if (text.view() == "\"") {
                return InsertAround { "\"", "\"" };
            }
            if (text.view() == "(") {
                return InsertAround { "(", ")" };
            }
            if (text.view() == "[") {
                return InsertAround { "[", "]" };
            }
            if (text.view() == "{") {
                return InsertAround { "{", "}" };
            }
            return {};
        }();

        if (insert_around_result) {
            auto selections = Vector<TextRange> {};
            for (auto& cursor : display.cursors()) {
                selections.add(cursor.selection());
            }

            auto group = make_unique<CommandGroup>(*this, "InsertAround");
            group->add<MovementCommand>(*this, [this, &selections](Display& display, MultiCursor& cursors) {
                for (int i = 0; i < cursors.size(); i++) {
                    auto& cursor = cursors[i];
                    move_cursor_to(display, cursor, selections[i].start());
                }
            });
            group->add<InsertCommand>(*this, insert_around_result->left);
            group->add<MovementCommand>(*this, [this, &selections](Display& display, MultiCursor& cursors) {
                for (int i = 0; i < cursors.size(); i++) {
                    auto& cursor = cursors[i];
                    move_cursor_to(display, cursor, selections[i].end().offset({ 0, 1 }));
                }
            });
            group->add<InsertCommand>(*this, insert_around_result->right);
            group->add<MovementCommand>(*this, [this, &selections](Display& display, MultiCursor& cursors) {
                for (int i = 0; i < cursors.size(); i++) {
                    auto& cursor = cursors[i];
                    move_cursor_to(display, cursor, selections[i].start().offset({ 0, 1 }));
                    move_cursor_to(display, cursor, selections[i].end().offset({ 0, 1 }), MovementMode::Select);
                }
            });
            return push_command(display, move(group));
        }
    }

    push_command<InsertCommand>(display, text);
}

void Document::replace_next_search_match(Display& display, const String& replacement) {
    if (display.search_results().empty()) {
        return;
    }

    auto group = make_unique<CommandGroup>(*this, "ReplaceSearchMatch");
    if (display.search_text() != selection_text(display.main_cursor())) {
        group->add<MovementCommand>(*this, [this](Display& display, MultiCursor&) {
            display.move_cursor_to_next_search_match();
        });
    }
    group->add<InsertCommand>(*this, replacement);
    if (display.search_results().size() > 1) {
        group->add<MovementCommand>(*this, [this](Display& display, MultiCursor&) {
            display.update_search_results();
            display.move_cursor_to_next_search_match();
        });
    }

    push_command(display, move(group));
}

void Document::replace_all_search_matches(Display& display, const String& replacement) {
    if (display.search_results().empty()) {
        return;
    }

    auto group = make_unique<CommandGroup>(*this, "ReplaceAll");
    group->add<MovementCommand>(*this, [this](Display& display, MultiCursor&) {
        select_all_matches(display, display.search_results());
    });
    group->add<InsertCommand>(*this, replacement);

    push_command(display, move(group));
}

void Document::move_cursor_to(Display& display, Cursor& cursor, const TextIndex& index, MovementMode mode) {
    update_selection_state_for_mode(cursor, mode);
    cursor.set(index);
    cursor.compute_max_col(display);
}

void Document::delete_selection(Cursor& cursor) {
    auto selection = cursor.selection();
    auto start = selection.start();
    auto end = selection.end();

    auto line_start = start.line_index();
    auto index_start = start.index_into_line();
    auto line_end = end.line_index();
    auto index_end = end.index_into_line();

    cursor.clear_selection();
    if (line_start == line_end) {
        for (int i = index_end - 1; i >= index_start; i--) {
            m_lines[line_start].remove_char_at(*this, { line_start, i });
        }
    } else {
        auto split_start = m_lines[line_start].split_at(index_start);
        auto split_end = m_lines[line_end].split_at(index_end);

        auto line_count_to_remove = line_end - line_start - 1;
        if (line_count_to_remove > 0) {
            remove_lines(line_start + 1, line_count_to_remove);
        }

        m_lines[line_start].overwrite(*this, move(split_start.first), line_start, Line::OverwriteFrom::LineEnd);
        m_lines[line_start + 1].overwrite(*this, move(split_end.second), line_start + 1, Line::OverwriteFrom::LineStart);

        merge_lines(line_start, line_start + 1);
    }
}

String Document::text_in_range(const TextIndex& start, const TextIndex& end) const {
    auto result = String {};
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

String Document::selection_text(const Cursor& cursor) const {
    auto selection = cursor.selection();
    if (selection.empty()) {
        return "";
    }

    return text_in_range(selection.start(), selection.end());
}

void Document::remove_lines(int line_index, int count) {
    m_lines.remove_count(line_index, count);
    emit<DeleteLines>(line_index, count);
}

void Document::remove_line(int index) {
    m_lines.remove(index);
    emit<DeleteLines>(index, 1);
}

void Document::insert_lines(int line_index, Span<StringView> lines) {
    auto lines_to_add = Vector<Line> {};
    for (auto& line : lines) {
        lines_to_add.add(Line { String { line } });
    }

    m_lines.insert(move(lines_to_add), line_index);
    emit<AddLines>(line_index, static_cast<int>(lines.size()));
}

void Document::insert_line(Line&& line, int index) {
    m_lines.insert(move(line), index);
    emit<AddLines>(index, 1);
}

void Document::merge_lines(int l1, int l2) {
    assert(l1 + 1 == l2);
    auto old_l1_length = m_lines[l1].length();
    m_lines[l1].combine_line(*this, m_lines[l2]);
    m_lines.remove(l2);
    emit<MergeLines>(l1, old_l1_length, l2);
}

void Document::split_line_at(const TextIndex& index) {
    auto& line = line_at_index(index.line_index());
    auto split_result = line.split_at(index.index_into_line());
    line.overwrite(*this, move(split_result.first), index.line_index(), Line::OverwriteFrom::None);
    m_lines.insert(move(split_result.second), index.line_index() + 1);
    emit<SplitLines>(index.line_index(), index.index_into_line());
}

void Document::move_line_to(int line, int destination) {
    if (line > destination) {
        m_lines.rotate_right(destination, line + 1);
    } else {
        m_lines.rotate_left(line, destination + 1);
    }
    emit<MoveLineTo>(line, destination);
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
        delete_line(display);
        return;
    }

    display.set_clipboard_contents(selection_text(cursor));
    push_command<DeleteCommand>(display);
}

void Document::paste(Display& display, MultiCursor& cursors) {
    bool is_whole_line;
    auto text_to_insert = display.clipboard_contents(is_whole_line);
    if (text_to_insert.empty()) {
        return;
    }

    auto& cursor = cursors.main_cursor();
    if (!input_text_mode() && cursor.selection().empty() && is_whole_line) {
        insert_line_at_cursor(display, text_to_insert);
    } else {
        insert_text_at_cursor(display, text_to_insert);
    }
}

void Document::invalidate_lines_in_range(Display& display, const TextRange& range) {
    if (range.start() == range.end()) {
        return;
    }

    bool exclusive = range.end().index_into_line() == 0;
    for (int i = range.start().line_index(); exclusive ? i < range.end().line_index() : i <= range.end().line_index(); i++) {
        display.invalidate_line(i);
    }
}

void Document::invalidate_lines_in_range_collection(Display& display, const TextRangeCollection& collection) {
    for (int i = 0; i < collection.size(); i++) {
        invalidate_lines_in_range(display, collection.range(i));
    }
}

App::ObjectBoundCoroutine Document::go_to_line(Display& display) {
    auto maybe_result = co_await display.prompt("Go to line: ");
    if (!maybe_result.has_value()) {
        co_return;
    }

    auto& result = maybe_result.value();
    char* end_ptr = result.string();
    long line_number = strtol(result.string(), &end_ptr, 10);
    if (errno == ERANGE || end_ptr != result.string() + result.size() || line_number < 1 || line_number > line_count()) {
        display.send_status_message(format("Line `{}' is not between 1 and {}", result, line_count()));
        co_return;
    }

    auto& cursor = display.main_cursor();

    cursor.clear_selection();
    cursor.set_line_index(line_number - 1);

    display.center_on_cursor(cursor);

    move_cursor_to_line_start(display, cursor);
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
    display.compute_suggestions();
}

App::ObjectBoundCoroutine Document::save(Display& display) {
    if (m_name.empty()) {
        auto result = co_await display.prompt("Save as: ");
        if (!result.has_value()) {
            co_return;
        }

        if (access(result.value().string(), F_OK) == 0) {
            auto ok = co_await display.prompt(format("Are you sure you want to overwrite file `{}'? ", *result));
            if (!ok.has_value() || (ok.value() != "y" && ok.value() != "yes")) {
                co_return;
            }
        }

        m_name = move(result.value());
        guess_type_from_name();
    }

    assert(!m_name.empty());

    if (access(m_name.string(), W_OK)) {
        if (errno != ENOENT) {
            display.send_status_message(format("Permission to write file `{}' denied", m_name));
            co_return;
        }
    }

    FILE* file = fopen(m_name.string(), "w");
    if (!file) {
        display.send_status_message(format("Failed to save - `{}'", strerror(errno)));
        co_return;
    }

    if (m_lines.size() != 1 || !m_lines.first().empty()) {
        for (auto& line : m_lines) {
            fprintf(file, "%s\n", line.contents().string());
        }
    }

    if (ferror(file)) {
        display.send_status_message(format("Failed to write to disk - `{}'", strerror(errno)));
        fclose(file);
        co_return;
    }

    if (fclose(file)) {
        display.send_status_message(format("Failed to sync to disk - `{}'", strerror(errno)));
        co_return;
    }

    display.send_status_message(format("Successfully saved file: `{}'", m_name.string()));
    m_document_was_modified = false;
}

App::ObjectBoundCoroutine Document::quit(Display& display) {
    if (m_document_was_modified && !input_text_mode()) {
        auto result = co_await display.prompt("Quit without saving? ");
        if (!result.has_value() || (result.value() != "y" && result.value() != "yes")) {
            co_return;
        }
    }
    display.quit();
}

void Document::update_syntax_highlighting() {
    highlight_document(*this);
    emit<SyntaxHighlightingChanged>();
}

void Document::swap_lines_at_cursor(Display& display, SwapDirection direction) {
    push_command<SwapLinesCommand>(display, direction);
}

void Document::select_all_matches(Display& display, const TextRangeCollection& collection) {
    if (collection.empty()) {
        return;
    }

    display.cursors().remove_secondary_cursors();

    auto& main_cursor = display.main_cursor();
    main_cursor.set_selection_anchor(collection.range(0).start());
    main_cursor.set(collection.range(0).end());

    for (int i = 1; i < collection.size(); i++) {
        auto& range = collection.range(i);
        display.cursors().add_cursor_at(*this, range.end(), range.start());
    }
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
    if (cursor.selection().empty()) {
        move_cursor_to_line_start(display, cursor, MovementMode::Move);
    }
    move_cursor_to_line_end(display, cursor, MovementMode::Select);
    move_cursor_right(display, cursor, MovementMode::Select);
}

void Document::select_all(Display& display, Cursor& cursor) {
    move_cursor_to_document_start(display, cursor, MovementMode::Move);
    move_cursor_to_document_end(display, cursor, MovementMode::Select);
}

void Document::push_command(Display& display, UniquePtr<Command> command) {
    bool did_modify = execute_command(display, *command);
    if (!did_modify) {
        return;
    }

    // This means some undo's have taken place, and the user started typing
    // something else, so the redo stack will be discarded.
    if (m_command_stack_index != m_command_stack.size()) {
        m_command_stack.resize(m_command_stack_index);
    }

    if (m_command_stack.empty() || !m_command_stack.last()->merge(*command)) {
        if (m_command_stack.size() >= m_max_undo_stack) {
            // FIXME: this makes the Vector data structure very inefficent
            //        a doubly-linked list would be much nicer.
            m_command_stack.remove(0);
            m_command_stack_index--;
        }

        m_command_stack.add(move(command));
        m_command_stack_index++;
    }

    m_document_was_modified = true;
    update_syntax_highlighting();
    update_suggestions(display);

    emit<Change>();
}

void Document::insert_suggestion(Display& display, const MatchedSuggestion& suggestion) {
    display.cursors().remove_secondary_cursors();
    move_cursor_to(display, display.main_cursor(), suggestion.start(), MovementMode::Select);
    insert_text_at_cursor(display, String { suggestion.content() });
}

void Document::start_input(Display& display, bool should_save_cursor_state) {
    if (should_save_cursor_state) {
        display.cursors().cursor_save();
    }
}

void Document::finish_input(Display& display, bool should_scroll_cursor_into_view) {
    auto& cursors = display.cursors();
    cursors.remove_duplicate_cursors();

    if (should_scroll_cursor_into_view) {
        display.scroll_cursor_into_view(cursors.main_cursor());
    }

    update_suggestions(display);
    if (display.preview_auto_complete()) {
        display.invalidate_line(cursors.main_cursor().line_index());
    }

    cursors.invalidate_based_on_last_snapshot(*this);
}
}
