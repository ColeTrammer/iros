#include "command.h"
#include "document.h"

Command::Command(Document& document) : m_document(document) {}

Command::~Command() {}

InsertCommand::InsertCommand(Document& document, char c) : Command(document), m_char(c) {}

InsertCommand::~InsertCommand() {}

void InsertCommand::execute() {
    auto& line = document().line_at_cursor();

    int col_position = document().cursor_col_position();
    int line_index = line.index_of_col_position(col_position);
    if (m_char == '\t' && document().convert_tabs_to_spaces()) {
        int num_spaces = tab_width - (col_position % tab_width);
        for (int i = 0; i < num_spaces; i++) {
            line.insert_char_at(line_index, ' ');
            document().move_cursor_right();
        }
    } else {
        line.insert_char_at(line_index, m_char);
        document().move_cursor_right();
    }
    document().set_needs_display();
    document().set_was_modified(true);
}

DeleteCommand::DeleteCommand(Document& document, DeleteCharMode mode) : Command(document), m_mode(mode) {}

DeleteCommand::~DeleteCommand() {}

void DeleteCommand::execute() {
    auto& line = document().line_at_cursor();
    int row_index = document().cursor_row_position();

    switch (m_mode) {
        case DeleteCharMode::Backspace: {
            if (line.empty()) {
                if (document().num_lines() == 1) {
                    return;
                }

                document().move_cursor_left();
                document().remove_line(row_index);
                document().set_needs_display();
                document().set_was_modified(true);
                return;
            }

            int index = line.index_of_col_position(document().cursor_col_position());
            if (index == 0) {
                if (row_index == 0) {
                    return;
                }

                document().move_cursor_up();
                document().move_cursor_to_line_end();
                document().merge_lines(row_index - 1, row_index);
            } else {
                document().move_cursor_left();
                line.remove_char_at(index - 1);
            }

            document().set_needs_display();
            document().set_was_modified(true);
            break;
        }
        case DeleteCharMode::Delete:
            if (line.empty()) {
                if (row_index == document().num_lines() - 1) {
                    return;
                }

                document().remove_line(row_index);
                document().set_needs_display();
                document().set_was_modified(true);
                return;
            }

            int index = line.index_of_col_position(document().cursor_col_position());
            if (index == line.length()) {
                if (row_index == document().num_lines() - 1) {
                    return;
                }

                document().merge_lines(row_index, row_index + 1);
            } else {
                line.remove_char_at(index);
            }

            document().set_needs_display();
            document().set_was_modified(true);
            break;
    }
}

SplitLineCommand::SplitLineCommand(Document& document) : Command(document) {}

SplitLineCommand::~SplitLineCommand() {}

void SplitLineCommand::execute() {
    auto& line = document().line_at_cursor();

    int row_index = document().cursor_row_position();
    auto result = line.split_at(line.index_of_col_position(document().cursor_col_position()));

    line = move(result.first);
    document().insert_line(move(result.second), row_index + 1);

    document().move_cursor_down();
    document().move_cursor_to_line_start();
    document().set_needs_display();
    document().set_was_modified(true);
}