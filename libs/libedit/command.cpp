#include <edit/command.h>
#include <edit/display.h>
#include <edit/document.h>
#include <edit/position.h>
#include <unistd.h>

namespace Edit {
Command::Command(Document& document) : m_document(document) {}

Command::~Command() {}

DeltaBackedCommand::DeltaBackedCommand(Document& document, Display& display)
    : Command(document), m_start_snapshot(document.snapshot_state(display)) {
    for (auto& cursor : m_start_snapshot.cursors) {
        m_selection_texts.add(document.selection_text(cursor));
    }
}

DeltaBackedCommand::~DeltaBackedCommand() {}

bool DeltaBackedCommand::execute(Display& display) {
    bool was_modified = do_execute(display.cursors());
    m_end_snapshot = document().snapshot_state(display);
    return was_modified;
}

void DeltaBackedCommand::undo(Display& display) {
    document().restore_state(display.cursors(), m_end_snapshot);
    do_undo(display.cursors());
    document().restore_state(display.cursors(), m_start_snapshot);
}

void DeltaBackedCommand::redo(Display& display) {
    document().restore_state(display.cursors(), start_snapshot());
    document().execute_command(display, *this);
}

SnapshotBackedCommand::SnapshotBackedCommand(Document& document, Display& display)
    : Command(document), m_snapshot(document.snapshot(display)) {}

SnapshotBackedCommand::~SnapshotBackedCommand() {}

void SnapshotBackedCommand::undo(Display& display) {
    document().restore(display.cursors(), snapshot());
}

void SnapshotBackedCommand::redo(Display& display) {
    document().restore_state(display.cursors(), snapshot().state);
    document().execute_command(display, *this);
}

InsertCommand::InsertCommand(Document& document, Display& display, String text)
    : DeltaBackedCommand(document, display), m_text(move(text)) {}

InsertCommand::~InsertCommand() {}

bool InsertCommand::do_execute(MultiCursor& cursors) {
    for (int i = 0; i < cursors.size(); i++) {
        auto& cursor = cursors[i];
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

        do_insert(document(), cursors, i, m_text);
    }

    return true;
}

void InsertCommand::do_insert(Document& document, MultiCursor& cursors, int cursor_index, char c) {
    auto& cursor = cursors[cursor_index];
    auto& line = cursor.referenced_line(document);
    if (c == '\n' || c == '\r') {
        document.split_line_at(cursor.index());
        return;
    }

    int index_into_line = cursor.index_into_line();
    if (c == '\t' && document.convert_tabs_to_spaces()) {
        // FIXME: what about variable width encodings/characters
        int num_spaces = tab_width - (index_into_line % tab_width);
        for (int i = 0; i < num_spaces; i++) {
            line.insert_char_at(document, index_into_line, ' ');
        }
    } else {
        line.insert_char_at(document, index_into_line, c);
    }
}

void InsertCommand::do_insert(Document& document, MultiCursor& cursors, int cursor_index, const String& text) {
    for (size_t i = 0; i < text.size(); i++) {
        do_insert(document, cursors, cursor_index, text[i]);
    }
}

void InsertCommand::do_undo(MultiCursor& cursors) {
    for (int cursor_index = 0; cursor_index < cursors.size(); cursor_index++) {
        auto& cursor = cursors[cursor_index];
        document().clear_selection(cursor);
        if (!start_snapshot().cursors[cursor_index].selection().empty()) {
            cursor.set(start_snapshot().cursors[cursor_index].selection().normalized_start());
        } else {
            cursor.set(start_snapshot().cursors[cursor_index].index());
        }

        for (size_t i = 0; i < m_text.size(); i++) {
            char c = m_text[i];
            if (c == '\n' || c == '\r') {
                document().merge_lines(cursor.line_index(), cursor.line_index() + 1);
            } else {
                auto& line = cursor.referenced_line(document());
                int index_into_line = cursor.index_into_line();

                // FIXME: what if convert_tabs_to_spaces() changes value
                if (c == '\t' && document().convert_tabs_to_spaces()) {
                    // FIXME: what about variable width encodings/characters
                    int num_spaces = tab_width - (index_into_line % tab_width);
                    for (int i = 0; i < num_spaces; i++) {
                        line.remove_char_at(document(), index_into_line);
                    }
                } else {
                    line.remove_char_at(document(), index_into_line);
                }
            }
        }

        if (!start_snapshot().cursors[cursor_index].selection().empty()) {
            do_insert(document(), cursors, cursor_index, selection_text(cursor_index));
        }
    }
}

DeleteCommand::DeleteCommand(Document& document, Display& display, DeleteCharMode mode)
    : DeltaBackedCommand(document, display), m_mode(mode) {}

DeleteCommand::~DeleteCommand() {}

bool DeleteCommand::do_execute(MultiCursor& cursors) {
    bool modified = false;
    for (int i = 0; i < cursors.size(); i++) {
        m_deleted_chars.add('\0');

        auto& cursor = cursors[i];
        if (!cursor.selection().empty()) {
            document().delete_selection(cursor);
            modified = true;
            continue;
        }

        auto& line = cursor.referenced_line(document());
        auto index = cursor.index();

        switch (m_mode) {
            case DeleteCharMode::Backspace: {
                if (index.index_into_line() == 0) {
                    if (index.line_index() == 0) {
                        continue;
                    }

                    document().merge_lines(index.line_index() - 1, index.line_index());
                    m_deleted_chars[i] = '\n';
                } else {
                    m_deleted_chars[i] = line.char_at(index.index_into_line() - 1);
                    line.remove_char_at(document(), index.index_into_line() - 1);
                }

                modified = true;
                continue;
            }
            case DeleteCharMode::Delete:
                if (index.index_into_line() == line.length()) {
                    if (index.line_index() == document().num_lines() - 1) {
                        continue;
                    }

                    document().merge_lines(index.line_index(), index.line_index() + 1);
                    m_deleted_chars[i] = '\n';
                } else {
                    m_deleted_chars[i] = line.char_at(index.index_into_line());
                    line.remove_char_at(document(), index.index_into_line());
                }

                modified = true;
                continue;
        }
    }

    return modified;
}

void DeleteCommand::do_undo(MultiCursor& cursors) {
    for (int i = cursors.size() - 1; i >= 0; i--) {
        auto& cursor = cursors[i];
        if (!start_snapshot().cursors[i].selection().empty()) {
            cursor.selection().clear();
            InsertCommand::do_insert(document(), cursors, i, selection_text(i));
        } else {
            assert(m_deleted_chars[i] != '\0');
            InsertCommand::do_insert(document(), cursors, i, m_deleted_chars[i]);
        }
    }
}

DeleteLineCommand::DeleteLineCommand(Document& document, Display& display) : DeltaBackedCommand(document, display) {}

DeleteLineCommand::~DeleteLineCommand() {}

bool DeleteLineCommand::do_execute(MultiCursor& cursors) {
    for (auto& cursor : cursors) {
        m_saved_lines.add(cursor.referenced_line(document()));

        int line_number = cursor.line_index();
        if (document().num_lines() == 1) {
            cursor.referenced_line(document()).overwrite(document(), Line(""), Line::OverwriteFrom::LineStart);
            m_document_was_empty = true;
        } else {
            document().remove_line(line_number);
        }
    }

    return true;
}

void DeleteLineCommand::do_undo(MultiCursor& cursors) {
    for (int i = 0; i < cursors.size(); i++) {
        document().insert_line(Line(m_saved_lines[i]), start_snapshot().cursors[i].line_index());
    }
    if (m_document_was_empty) {
        document().remove_line(1);
    }
}

InsertLineCommand::InsertLineCommand(Document& document, Display& display, String text)
    : DeltaBackedCommand(document, display), m_text(move(text)) {}

InsertLineCommand::~InsertLineCommand() {}

bool InsertLineCommand::do_execute(MultiCursor& cursors) {
    Line to_add(m_text);
    auto& cursor = cursors.main_cursor();
    document().insert_line(move(to_add), cursor.line_index());
    return true;
}

void InsertLineCommand::do_undo(MultiCursor& cursors) {
    document().remove_line(cursors.main_cursor().line_index());
}

SwapLinesCommand::SwapLinesCommand(Document& document, Display& display, SwapDirection direction)
    : DeltaBackedCommand(document, display), m_direction(direction) {}

SwapLinesCommand::~SwapLinesCommand() {}

bool SwapLinesCommand::do_execute(MultiCursor& cursors) {
    cursors.remove_secondary_cursors();
    auto& cursor = cursors.main_cursor();
    bool ret = do_swap(cursor, m_direction);
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

    return true;
}

void SwapLinesCommand::do_undo(MultiCursor& cursors) {
    auto& cursor = cursors.main_cursor();
    do_swap(cursor, m_direction == SwapDirection::Up ? SwapDirection::Down : SwapDirection::Up);
}
}
