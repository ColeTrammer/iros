#include <edit/command.h>
#include <edit/document.h>
#include <edit/panel.h>

namespace Edit {
Command::Command(Document& document) : m_document(document) {}

Command::~Command() {}

DeltaBackedCommand::DeltaBackedCommand(Document& document) : Command(document), m_snapshot(document.snapshot_state()) {
    m_selection_text = document.selection_text();
}

DeltaBackedCommand::~DeltaBackedCommand() {}

void DeltaBackedCommand::redo() {
    document().restore_state(state_snapshot());
    document().execute_command(*this);
}

SnapshotBackedCommand::SnapshotBackedCommand(Document& document) : Command(document), m_snapshot(document.snapshot()) {}

SnapshotBackedCommand::~SnapshotBackedCommand() {}

void SnapshotBackedCommand::undo() {
    document().restore(snapshot());
}

void SnapshotBackedCommand::redo() {
    document().restore_state(snapshot().state);
    document().execute_command(*this);
}

InsertCommand::InsertCommand(Document& document, String text) : DeltaBackedCommand(document), m_text(move(text)) {}

InsertCommand::~InsertCommand() {}

bool InsertCommand::execute() {
    if (!document().selection().empty()) {
        document().delete_selection();
    }

    if (m_text.size() == 1 && m_text[0] == '\n') {
        String leading_whitespace = "";
        auto& line = document().line_at_cursor();
        for (int i = 0; i < line.length(); i++) {
            if (!isspace(line.char_at(i))) {
                break;
            }

            leading_whitespace += String(line.char_at(i));
        }
        m_text += move(leading_whitespace);
    }

    do_insert(document(), m_text);

    document().set_needs_display();
    return true;
}

void InsertCommand::do_insert(Document& document, char c) {
    auto& line = document.line_at_cursor();
    if (c == '\n') {
        int line_index = document.index_of_line_at_cursor();
        auto result = line.split_at(document.index_into_line_at_cursor());

        line = move(result.first);
        document.insert_line(move(result.second), line_index + 1);
        document.move_cursor_down();
        document.move_cursor_to_line_start();
        document.set_needs_display();
        return;
    }

    int index_into_line = document.index_into_line_at_cursor();
    if (c == '\t' && document.convert_tabs_to_spaces()) {
        int num_spaces = tab_width - (line.col_position_of_index(document, document.panel(), index_into_line) % tab_width);
        for (int i = 0; i < num_spaces; i++) {
            line.insert_char_at(index_into_line, ' ');
            document.move_cursor_right();
        }
    } else {
        line.insert_char_at(index_into_line, c);
        document.move_cursor_right();
    }
}

void InsertCommand::do_insert(Document& document, const String& text) {
    for (size_t i = 0; i < text.size(); i++) {
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

    for (size_t i = 0; i < m_text.size(); i++) {
        char c = m_text[i];
        if (c == '\n') {
            int line_index = document().index_of_line_at_cursor();
            document().merge_lines(line_index, line_index + 1);
        } else {
            auto& line = document().line_at_cursor();
            int index_into_line = document().index_into_line_at_cursor();

            // FIXME: what if convert_tabs_to_spaces() changes value
            if (c == '\t' && document().convert_tabs_to_spaces()) {
                int num_spaces = tab_width - (line.col_position_of_index(document(), document().panel(), index_into_line) % tab_width);
                for (int i = 0; i < num_spaces; i++) {
                    line.remove_char_at(index_into_line);
                }
            } else {
                line.remove_char_at(index_into_line);
            }

            document().set_needs_display();
        }
    }

    if (!state_snapshot().selection.empty()) {
        do_insert(document(), selection_text());
        document().restore_state(state_snapshot());
    }
}

DeleteCommand::DeleteCommand(Document& document, DeleteCharMode mode, bool should_clear_selection)
    : DeltaBackedCommand(document), m_mode(mode), m_should_clear_selection(should_clear_selection) {}

DeleteCommand::~DeleteCommand() {}

bool DeleteCommand::execute() {
    if (!document().selection().empty()) {
        document().delete_selection();
        return true;
    }

    auto& line = document().line_at_cursor();
    int line_index = document().index_of_line_at_cursor();

    switch (m_mode) {
        case DeleteCharMode::Backspace: {
            if (line.empty()) {
                if (document().num_lines() == 1) {
                    return false;
                }

                document().move_cursor_left();
                document().remove_line(line_index);
                document().set_needs_display();
                m_end_line = line_index - 1;
                m_end_index = document().line_at_cursor().length();
                m_deleted_char = '\n';
                return true;
            }

            int index = document().index_into_line_at_cursor();
            if (index == 0) {
                if (line_index == 0) {
                    return false;
                }

                document().move_cursor_up();
                document().move_cursor_to_line_end();
                document().merge_lines(line_index - 1, line_index);
                m_end_line = line_index - 1;
                m_end_index = document().index_into_line_at_cursor();
                m_deleted_char = '\n';
            } else {
                document().move_cursor_left();
                m_end_line = line_index;
                m_end_index = index - 1;
                m_deleted_char = line.char_at(index - 1);
                line.remove_char_at(index - 1);
            }

            document().set_needs_display();
            return true;
        }
        case DeleteCharMode::Delete:
            int index = document().index_into_line_at_cursor();
            m_end_line = line_index;
            m_end_index = index;

            if (line.empty()) {
                if (line_index == document().num_lines() - 1) {
                    return false;
                }

                document().remove_line(line_index);
                document().set_needs_display();
                m_deleted_char = '\n';
                return true;
            }

            if (index == line.length()) {
                if (line_index == document().num_lines() - 1) {
                    return false;
                }

                document().merge_lines(line_index, line_index + 1);
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
    if (m_should_clear_selection) {
        document().clear_selection();
    }
}

DeleteLineCommand::DeleteLineCommand(Document& document) : DeltaBackedCommand(document), m_saved_line("") {}

DeleteLineCommand::~DeleteLineCommand() {}

bool DeleteLineCommand::execute() {
    m_saved_line = document().line_at_cursor();

    int line_number = document().index_of_line_at_cursor();
    bool deleted_last_line = false;
    if (line_number != 0 && line_number == document().num_lines() - 1) {
        deleted_last_line = true;
        document().move_cursor_up();
        document().move_cursor_to_line_end();
    }

    document().remove_line(line_number);
    if (document().num_lines() == 0) {
        m_document_was_empty = true;
        document().insert_line(Line(""), 0);
    }

    if (!deleted_last_line) {
        document().move_cursor_to_line_start();
    }

    document().set_needs_display();
    return true;
}

void DeleteLineCommand::undo() {
    document().restore_state(state_snapshot());
    document().insert_line(Line(m_saved_line), document().index_of_line_at_cursor());
    if (m_document_was_empty) {
        document().remove_line(1);
    }
}

InsertLineCommand::InsertLineCommand(Document& document, String text) : DeltaBackedCommand(document), m_text(move(text)) {}

InsertLineCommand::~InsertLineCommand() {}

bool InsertLineCommand::execute() {
    Line to_add(m_text);
    document().insert_line(move(to_add), document().index_of_line_at_cursor());
    document().move_cursor_down();
    document().set_needs_display();
    return true;
}

void InsertLineCommand::undo() {
    document().restore_state(state_snapshot());
    document().remove_line(document().index_of_line_at_cursor());
}

SwapLinesCommand::SwapLinesCommand(Document& document, SwapDirection direction) : DeltaBackedCommand(document), m_direction(direction) {}

SwapLinesCommand::~SwapLinesCommand() {}

bool SwapLinesCommand::execute() {
    bool ret = do_swap(m_direction);
    m_end_line = document().index_of_line_at_cursor();
    m_end_index = document().index_into_line_at_cursor();
    m_end_selection = document().selection();
    return ret;
}

bool SwapLinesCommand::do_swap(SwapDirection direction) {
    if (document().selection().empty()) {
        int line_index = document().index_of_line_at_cursor();
        if ((line_index == 0 && direction == SwapDirection::Up) ||
            (line_index == document().num_lines() - 1 && direction == SwapDirection::Down)) {
            return false;
        }

        if (direction == SwapDirection::Up) {
            document().rotate_lines_up(line_index - 1, line_index);
            document().move_cursor_up();
        } else {
            document().rotate_lines_down(line_index, line_index + 1);
            document().move_cursor_down();
        }

        document().set_needs_display();
        return true;
    }

    int selection_start_line = document().selection().upper_line();
    int selection_end_line = document().selection().lower_line();

    if ((selection_start_line == 0 && direction == SwapDirection::Up) ||
        (selection_end_line == document().num_lines() - 1 && direction == SwapDirection::Down)) {
        return false;
    }

    if (direction == SwapDirection::Up) {
        document().rotate_lines_up(selection_start_line - 1, selection_end_line);
        auto selection = document().selection();
        const_cast<Selection&>(document().selection()).clear();

        document().move_cursor_up();
        selection.set_start_line(selection_start_line - 1);
        selection.set_end_line(selection_end_line - 1);

        document().set_selection(move(selection));
    } else {
        document().rotate_lines_down(selection_start_line, selection_end_line + 1);
        auto selection = document().selection();
        const_cast<Selection&>(document().selection()).clear();

        document().move_cursor_down();
        selection.set_start_line(selection_start_line + 1);
        selection.set_end_line(selection_end_line + 1);

        document().set_selection(move(selection));
    }

    document().set_needs_display();
    return true;
}

void SwapLinesCommand::undo() {
    document().move_cursor_to(m_end_line, m_end_index);
    document().set_selection(m_end_selection);
    do_swap(m_direction == SwapDirection::Up ? SwapDirection::Down : SwapDirection::Up);
    document().restore_state(state_snapshot());
}
}
