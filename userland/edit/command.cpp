#include "command.h"
#include "document.h"

Command::Command(Document& document) : m_document(document) {}

Command::~Command() {}

DeltaBackedCommand::DeltaBackedCommand(Document& document) : Command(document), m_snapshot(document.snapshot_state()) {
    m_selection_text = document.selection_text();
}

DeltaBackedCommand::~DeltaBackedCommand() {}

void DeltaBackedCommand::redo() {
    document().restore_state(state_snapshot());
    execute();
}

SnapshotBackedCommand::SnapshotBackedCommand(Document& document) : Command(document), m_snapshot(document.snapshot()) {}

SnapshotBackedCommand::~SnapshotBackedCommand() {}

void SnapshotBackedCommand::undo() {
    document().restore(snapshot());
}

void SnapshotBackedCommand::redo() {
    document().restore_state(snapshot().state);
    execute();
}

InsertCommand::InsertCommand(Document& document, String text) : DeltaBackedCommand(document), m_text(move(text)) {}

InsertCommand::~InsertCommand() {}

bool InsertCommand::execute() {
    if (!document().selection().empty()) {
        document().delete_selection();
    }

    do_insert(document(), m_text);

    document().set_needs_display();
    return true;
}

void InsertCommand::do_insert(Document& document, char c) {
    auto& line = document.line_at_cursor();
    if (c == '\n') {
        int row_index = document.cursor_row_position();
        auto result = line.split_at(line.index_of_col_position(document.cursor_col_position()));

        line = move(result.first);
        document.insert_line(move(result.second), row_index + 1);
        document.move_cursor_down();
        document.move_cursor_to_line_start();
        document.set_needs_display();
        return;
    }

    int col_position = document.cursor_col_position();
    int line_index = line.index_of_col_position(col_position);
    if (c == '\t' && document.convert_tabs_to_spaces()) {
        int num_spaces = tab_width - (col_position % tab_width);
        for (int i = 0; i < num_spaces; i++) {
            line.insert_char_at(line_index, ' ');
            document.move_cursor_right();
        }
    } else {
        line.insert_char_at(line_index, c);
        document.move_cursor_right();
    }
}

void InsertCommand::do_insert(Document& document, const String& text) {
    for (int i = 0; i < text.size(); i++) {
        do_insert(document, text[i]);
    }
}

void InsertCommand::undo() {
    document().clear_selection();
    if (!state_snapshot().selection.empty()) {
        document().move_cursor_to(state_snapshot().selection.upper_line(), state_snapshot().selection.upper_index());
    } else {
        document().restore_state(state_snapshot());
    }

    for (int i = 0; i < m_text.size(); i++) {
        char c = m_text[i];
        if (c == '\n') {
            int row_index = document().cursor_row_position();
            document().merge_lines(row_index, row_index + 1);
        } else {
            auto& line = document().line_at_cursor();
            int col_position = document().cursor_col_position();
            int line_index = line.index_of_col_position(col_position);

            // FIXME: what if convert_tabs_to_spaces() changes value
            if (c == '\t' && document().convert_tabs_to_spaces()) {
                int num_spaces = tab_width - (col_position % tab_width);
                for (int i = 0; i < num_spaces; i++) {
                    line.remove_char_at(line_index);
                }
            } else {
                line.remove_char_at(line_index);
            }

            document().set_needs_display();
        }
    }

    if (!state_snapshot().selection.empty()) {
        do_insert(document(), selection_text());
        document().restore_state(state_snapshot());
    }
}

DeleteCommand::DeleteCommand(Document& document, DeleteCharMode mode) : DeltaBackedCommand(document), m_mode(mode) {}

DeleteCommand::~DeleteCommand() {}

bool DeleteCommand::execute() {
    if (!document().selection().empty()) {
        document().delete_selection();
        return true;
    }

    auto& line = document().line_at_cursor();
    int row_index = document().cursor_row_position();

    switch (m_mode) {
        case DeleteCharMode::Backspace: {
            if (line.empty()) {
                if (document().num_lines() == 1) {
                    return false;
                }

                document().move_cursor_left();
                document().remove_line(row_index);
                document().set_needs_display();
                m_end_line = row_index - 1;
                m_end_index = document().line_at_cursor().length();
                m_deleted_char = '\n';
                return true;
            }

            int index = line.index_of_col_position(document().cursor_col_position());
            if (index == 0) {
                if (row_index == 0) {
                    return false;
                }

                document().move_cursor_up();
                document().move_cursor_to_line_end();
                document().merge_lines(row_index - 1, row_index);
                m_end_line = row_index - 1;
                m_end_index = document().line_index_at_cursor();
                m_deleted_char = '\n';
            } else {
                document().move_cursor_left();
                m_end_line = row_index;
                m_end_index = index - 1;
                m_deleted_char = line.char_at(index - 1);
                line.remove_char_at(index - 1);
            }

            document().set_needs_display();
            return true;
        }
        case DeleteCharMode::Delete:
            int index = line.index_of_col_position(document().cursor_col_position());
            m_end_line = row_index;
            m_end_index = index;

            if (line.empty()) {
                if (row_index == document().num_lines() - 1) {
                    return false;
                }

                document().remove_line(row_index);
                document().set_needs_display();
                m_deleted_char = '\n';
                return true;
            }

            if (index == line.length()) {
                if (row_index == document().num_lines() - 1) {
                    return false;
                }

                document().merge_lines(row_index, row_index + 1);
                m_deleted_char = '\n';
            } else {
                m_deleted_char = line.char_at(index);
                line.remove_char_at(index);
            }

            document().set_needs_display();
            return true;
    }

    return false;
}

void DeleteCommand::undo() {
    document().clear_selection();
    if (!state_snapshot().selection.empty()) {
        document().move_cursor_to(state_snapshot().selection.upper_line(), state_snapshot().selection.upper_index());
        InsertCommand::do_insert(document(), selection_text());
    } else {
        assert(m_deleted_char != '\0');
        document().move_cursor_to(m_end_line, m_end_index);
        InsertCommand::do_insert(document(), m_deleted_char);
    }

    document().restore_state(state_snapshot());
}

DeleteLineCommand::DeleteLineCommand(Document& document) : DeltaBackedCommand(document), m_saved_line("") {}

DeleteLineCommand::~DeleteLineCommand() {}

bool DeleteLineCommand::execute() {
    m_saved_line = document().line_at_cursor();
    document().remove_line(document().cursor_row_position());
    document().move_cursor_to_line_start();
    document().set_needs_display();
    return true;
}

void DeleteLineCommand::undo() {
    document().restore_state(state_snapshot());
    document().insert_line(Line(m_saved_line), document().cursor_row_position());
}

InsertLineCommand::InsertLineCommand(Document& document, String text) : DeltaBackedCommand(document), m_text(move(text)) {}

InsertLineCommand::~InsertLineCommand() {}

bool InsertLineCommand::execute() {
    Line to_add(m_text);
    document().insert_line(move(to_add), document().cursor_row_position());
    document().move_cursor_down();
    document().set_needs_display();
    return true;
}

void InsertLineCommand::undo() {
    document().restore_state(state_snapshot());
    document().remove_line(document().cursor_row_position());
}