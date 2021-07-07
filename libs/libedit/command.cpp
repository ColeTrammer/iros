#include <edit/command.h>
#include <edit/document.h>
#include <edit/panel.h>
#include <edit/position.h>

namespace Edit {
Command::Command(Document& document) : m_document(document) {}

Command::~Command() {}

DeltaBackedCommand::DeltaBackedCommand(Document& document) : Command(document), m_snapshot(document.snapshot_state()) {
    m_selection_text = document.selection_text(m_snapshot.cursor);
}

DeltaBackedCommand::~DeltaBackedCommand() {}

void DeltaBackedCommand::redo(Cursor& cursor) {
    document().restore_state(cursor, state_snapshot());
    document().execute_command(cursor, *this);
}

SnapshotBackedCommand::SnapshotBackedCommand(Document& document) : Command(document), m_snapshot(document.snapshot()) {}

SnapshotBackedCommand::~SnapshotBackedCommand() {}

void SnapshotBackedCommand::undo(Cursor& cursor) {
    document().restore(cursor, snapshot());
}

void SnapshotBackedCommand::redo(Cursor& cursor) {
    document().restore_state(cursor, snapshot().state);
    document().execute_command(cursor, *this);
}

InsertCommand::InsertCommand(Document& document, String text) : DeltaBackedCommand(document), m_text(move(text)) {}

InsertCommand::~InsertCommand() {}

bool InsertCommand::execute(Cursor& cursor) {
    if (!cursor.selection().empty()) {
        document().delete_selection(cursor);
    }

    if (m_text.size() == 1 && m_text[0] == '\n') {
        String leading_whitespace = "";
        auto& line = cursor.referenced_line(document());
        for (int i = 0; i < line.length(); i++) {
            if (!isspace(line.char_at(i))) {
                break;
            }

            leading_whitespace += String(line.char_at(i));
        }
        m_text += move(leading_whitespace);
    }

    do_insert(document(), cursor, m_text);

    document().set_needs_display();
    return true;
}

void InsertCommand::do_insert(Document& document, Cursor& cursor, char c) {
    auto& line = cursor.referenced_line(document);
    if (c == '\n') {
        int line_index = cursor.line_index();
        auto result = line.split_at(cursor.index_into_line());

        line = move(result.first);
        document.insert_line(move(result.second), line_index + 1);
        document.move_cursor_down(cursor);
        document.move_cursor_to_line_start(cursor);
        document.set_needs_display();
        return;
    }

    int index_into_line = cursor.index_into_line();
    if (c == '\t' && document.convert_tabs_to_spaces()) {
        int num_spaces = tab_width - (line.absoulte_col_offset_of_index(document, document.panel(), index_into_line) % tab_width);
        for (int i = 0; i < num_spaces; i++) {
            line.insert_char_at(index_into_line, ' ');
            document.move_cursor_right(cursor);
        }
    } else {
        line.insert_char_at(index_into_line, c);
        document.move_cursor_right(cursor);
    }
}

void InsertCommand::do_insert(Document& document, Cursor& cursor, const String& text) {
    for (size_t i = 0; i < text.size(); i++) {
        do_insert(document, cursor, text[i]);
    }
}

void InsertCommand::undo(Cursor& cursor) {
    document().clear_selection(cursor);
    if (!state_snapshot().cursor.selection().empty()) {
        document().move_cursor_to(cursor, state_snapshot().cursor.selection().normalized_start());
    } else {
        document().restore_state(cursor, state_snapshot());
    }

    for (size_t i = 0; i < m_text.size(); i++) {
        char c = m_text[i];
        if (c == '\n') {
            int line_index = cursor.line_index();
            document().merge_lines(line_index, line_index + 1);
        } else {
            auto& line = cursor.referenced_line(document());
            int index_into_line = cursor.index_into_line();

            // FIXME: what if convert_tabs_to_spaces() changes value
            if (c == '\t' && document().convert_tabs_to_spaces()) {
                int num_spaces =
                    tab_width - (line.absoulte_col_offset_of_index(document(), document().panel(), index_into_line) % tab_width);
                for (int i = 0; i < num_spaces; i++) {
                    line.remove_char_at(index_into_line);
                }
            } else {
                line.remove_char_at(index_into_line);
            }

            document().set_needs_display();
        }
    }

    if (!state_snapshot().cursor.selection().empty()) {
        do_insert(document(), cursor, selection_text());
        document().restore_state(cursor, state_snapshot());
    }
}

DeleteCommand::DeleteCommand(Document& document, DeleteCharMode mode, bool should_clear_selection)
    : DeltaBackedCommand(document), m_mode(mode), m_should_clear_selection(should_clear_selection) {}

DeleteCommand::~DeleteCommand() {}

bool DeleteCommand::execute(Cursor& cursor) {
    if (!cursor.selection().empty()) {
        document().delete_selection(cursor);
        return true;
    }

    auto& line = cursor.referenced_line(document());
    auto index = cursor.index();

    switch (m_mode) {
        case DeleteCharMode::Backspace: {
            if (line.empty()) {
                if (document().num_lines() == 1) {
                    return false;
                }

                document().move_cursor_left(cursor);
                document().remove_line(index.line_index());
                document().set_needs_display();
                m_end = { index.line_index() - 1, cursor.referenced_line(document()).length() };
                m_deleted_char = '\n';
                return true;
            }

            if (index.index_into_line() == 0) {
                if (index.line_index() == 0) {
                    return false;
                }

                document().move_cursor_up(cursor);
                document().move_cursor_to_line_end(cursor);
                document().merge_lines(index.line_index() - 1, index.line_index());
                m_end = { index.line_index() - 1, cursor.referenced_line(document()).length() };
                m_deleted_char = '\n';
            } else {
                document().move_cursor_left(cursor);
                m_end = { index.line_index(), index.index_into_line() - 1 };
                m_deleted_char = line.char_at(index.index_into_line() - 1);
                line.remove_char_at(index.index_into_line() - 1);
            }

            document().set_needs_display();
            return true;
        }
        case DeleteCharMode::Delete:
            m_end = index;

            if (line.empty()) {
                if (index.line_index() == document().num_lines() - 1) {
                    return false;
                }

                document().remove_line(index.line_index());
                document().set_needs_display();
                m_deleted_char = '\n';
                return true;
            }

            if (index.index_into_line() == line.length()) {
                if (index.line_index() == document().num_lines() - 1) {
                    return false;
                }

                document().merge_lines(index.line_index(), index.line_index() + 1);
                m_deleted_char = '\n';
            } else {
                m_deleted_char = line.char_at(index.index_into_line());
                line.remove_char_at(index.index_into_line());
            }

            document().set_needs_display();
            return true;
    }

    return false;
}

void DeleteCommand::undo(Cursor& cursor) {
    document().clear_selection(cursor);
    if (!state_snapshot().cursor.selection().empty()) {
        document().move_cursor_to(cursor, state_snapshot().cursor.selection().normalized_start());
        InsertCommand::do_insert(document(), cursor, selection_text());
    } else {
        assert(m_deleted_char != '\0');
        document().move_cursor_to(cursor, m_end);
        InsertCommand::do_insert(document(), cursor, m_deleted_char);
    }

    document().restore_state(cursor, state_snapshot());
    if (m_should_clear_selection) {
        document().clear_selection(cursor);
    }
}

DeleteLineCommand::DeleteLineCommand(Document& document) : DeltaBackedCommand(document), m_saved_line("") {}

DeleteLineCommand::~DeleteLineCommand() {}

bool DeleteLineCommand::execute(Cursor& cursor) {
    m_saved_line = cursor.referenced_line(document());

    int line_number = cursor.line_index();
    bool deleted_last_line = false;
    if (line_number != 0 && line_number == document().num_lines() - 1) {
        deleted_last_line = true;
        document().move_cursor_up(cursor);
        document().move_cursor_to_line_end(cursor);
    }

    document().remove_line(line_number);
    if (document().num_lines() == 0) {
        m_document_was_empty = true;
        document().insert_line(Line(""), 0);
    }

    if (!deleted_last_line) {
        document().move_cursor_to_line_start(cursor);
    }

    document().set_needs_display();
    return true;
}

void DeleteLineCommand::undo(Cursor& cursor) {
    document().restore_state(cursor, state_snapshot());
    document().insert_line(Line(m_saved_line), cursor.line_index());
    if (m_document_was_empty) {
        document().remove_line(1);
    }
}

InsertLineCommand::InsertLineCommand(Document& document, String text) : DeltaBackedCommand(document), m_text(move(text)) {}

InsertLineCommand::~InsertLineCommand() {}

bool InsertLineCommand::execute(Cursor& cursor) {
    Line to_add(m_text);
    document().insert_line(move(to_add), cursor.line_index());
    document().move_cursor_down(cursor);
    document().set_needs_display();
    return true;
}

void InsertLineCommand::undo(Cursor& cursor) {
    document().restore_state(cursor, state_snapshot());
    document().remove_line(cursor.line_index());
}

SwapLinesCommand::SwapLinesCommand(Document& document, SwapDirection direction) : DeltaBackedCommand(document), m_direction(direction) {}

SwapLinesCommand::~SwapLinesCommand() {}

bool SwapLinesCommand::execute(Cursor& cursor) {
    bool ret = do_swap(cursor, m_direction);
    m_end = cursor.index();
    m_end_selection = cursor.selection();
    return ret;
}

bool SwapLinesCommand::do_swap(Cursor& cursor, SwapDirection direction) {
    if (cursor.selection().empty()) {
        int line_index = cursor.line_index();
        if ((line_index == 0 && direction == SwapDirection::Up) ||
            (line_index == document().num_lines() - 1 && direction == SwapDirection::Down)) {
            return false;
        }

        if (direction == SwapDirection::Up) {
            document().rotate_lines_up(line_index - 1, line_index);
            cursor.set_line_index(line_index - 1);
        } else {
            document().rotate_lines_down(line_index, line_index + 1);
            cursor.set_line_index(line_index + 1);
        }

        document().set_needs_display();
        return true;
    }

    auto selection_start = cursor.selection().normalized_start();
    auto selection_end = cursor.selection().normalized_end();

    if ((selection_start.line_index() == 0 && direction == SwapDirection::Up) ||
        (selection_end.line_index() == document().num_lines() - 1 && direction == SwapDirection::Down)) {
        return false;
    }

    if (direction == SwapDirection::Up) {
        document().rotate_lines_up(selection_start.line_index() - 1, selection_end.line_index());
        auto selection = cursor.selection();
        const_cast<Selection&>(cursor.selection()).clear();

        cursor.set_line_index(selection_start.line_index() - 1);
        selection.set_start_line_index(selection_start.line_index() - 1);
        selection.set_end_line_index(selection_end.line_index() - 1);

        cursor.selection() = move(selection);
    } else {
        document().rotate_lines_down(selection_start.line_index(), selection_end.line_index() + 1);
        auto selection = cursor.selection();
        const_cast<Selection&>(cursor.selection()).clear();

        cursor.set_line_index(selection_start.line_index() + 1);
        selection.set_start_line_index(selection_start.line_index() + 1);
        selection.set_end_line_index(selection_end.line_index() + 1);

        cursor.selection() = move(selection);
    }

    document().set_needs_display();
    return true;
}

void SwapLinesCommand::undo(Cursor& cursor) {
    document().move_cursor_to(cursor, m_end);
    cursor.selection() = m_end_selection;
    do_swap(cursor, m_direction == SwapDirection::Up ? SwapDirection::Down : SwapDirection::Up);
    document().restore_state(cursor, state_snapshot());
}
}
