#include <edit/command.h>
#include <edit/display.h>
#include <edit/document.h>
#include <unistd.h>

namespace Edit {
Command::Command(Document& document) : m_document(document) {}

Command::~Command() {}

DeltaBackedCommand::DeltaBackedCommand(Document& document) : Command(document) {}

DeltaBackedCommand::~DeltaBackedCommand() {}

bool DeltaBackedCommand::execute(Display& display) {
    m_start_snapshot = document().snapshot_state(display);
    for (auto& cursor : m_start_snapshot.cursors) {
        m_selection_texts.add(cursor.selection_text(document()));
    }
    bool was_modified = do_execute(display, display.cursors());
    m_end_snapshot = document().snapshot_state(display);
    return was_modified;
}

void DeltaBackedCommand::undo(Display& display) {
    document().restore_state(display.cursors(), m_end_snapshot);
    do_undo(display, display.cursors());
    document().restore_state(display.cursors(), m_start_snapshot);
}

void DeltaBackedCommand::redo(Display& display) {
    document().restore_state(display.cursors(), start_snapshot());
    do_execute(display, display.cursors());
}

bool CommandGroup::merge(Command& other_command) {
    if (m_should_merge == ShouldMerge::No) {
        return false;
    }

    if (other_command.name() != name()) {
        return false;
    }

    auto& other = static_cast<CommandGroup&>(other_command);
    if (other.start_snapshot().cursors != this->end_snapshot().cursors) {
        return false;
    }

    m_commands.add(move(other.m_commands));
    set_end_snapshot(other.end_snapshot());
    return true;
}

bool CommandGroup::do_execute(Display& display, MultiCursor&) {
    bool modified = false;
    for (auto& command : m_commands) {
        if (command->execute(display)) {
            modified = true;
        }
    }
    return modified;
}

void CommandGroup::do_undo(Display& display, MultiCursor&) {
    m_commands.for_each_reverse([&](auto& command) {
        const_cast<Command&>(*command).undo(display);
    });
}

void CommandGroup::redo(Display& display) {
    for (auto& command : m_commands) {
        command->redo(display);
    }
}

void MovementCommand::redo(Display& display) {
    document().restore_state(display.cursors(), end_snapshot());
}

InsertCommand::InsertCommand(Document& document, String text) : DeltaBackedCommand(document), m_text(move(text)) {}

InsertCommand::~InsertCommand() {}

bool InsertCommand::merge(Command& other_command) {
    if (other_command.name() != name()) {
        return false;
    }

    auto& other = static_cast<InsertCommand&>(other_command);
    if (other.start_snapshot().cursors != end_snapshot().cursors) {
        return false;
    }

    m_text += other.m_text;
    set_end_snapshot(other.end_snapshot());
    return true;
}

bool InsertCommand::do_execute(Display&, MultiCursor& cursors) {
    if (m_text.empty()) {
        return false;
    }

    for (int i = 0; i < cursors.size(); i++) {
        auto& cursor = cursors[i];
        if (!cursor.selection().empty()) {
            document().delete_text_in_range(cursor.selection());
        }

        if (m_text.size() == 1 && m_text[0] == '\n') {
            String leading_whitespace = "";
            auto& line = cursor.referenced_line(document());
            for (int i = 0; i < cursor.index_into_line(); i++) {
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

    if (c == '\t' && document.convert_tabs_to_spaces()) {
        // FIXME: what about variable width encodings/characters
        int num_spaces = tab_width - (cursor.index_into_line() % tab_width);
        for (int i = 0; i < num_spaces; i++) {
            line.insert_char_at(document, cursor.index(), ' ');
        }
    } else {
        line.insert_char_at(document, cursor.index(), c);
    }
}

void InsertCommand::do_insert(Document& document, MultiCursor& cursors, int cursor_index, const String& text) {
    if (text.size() == 1) {
        return do_insert(document, cursors, cursor_index, text.first());
    }

    auto& cursor = cursors[cursor_index];

    auto lines = text.split_view("\r\n", SplitMethod::KeepEmpty);
    assert(!lines.empty());

    auto& first_line = cursor.referenced_line(document);
    for (auto& c : lines.first()) {
        first_line.insert_char_at(document, cursor.index(), c);
    }
    if (lines.size() > 1) {
        document.split_line_at(cursor.index());
    }

    if (lines.size() > 2) {
        document.insert_lines(cursor.line_index(), lines.span().subspan(1, lines.size() - 2));
    }

    if (lines.size() > 1) {
        auto& last_line = cursor.referenced_line(document);
        for (auto& c : lines.last()) {
            last_line.insert_char_at(document, cursor.index(), c);
        }
    }
}

void InsertCommand::do_undo(Display&, MultiCursor& cursors) {
    for (int cursor_index = 0; cursor_index < cursors.size(); cursor_index++) {
        auto& cursor = cursors[cursor_index];

        cursor.set_selection_anchor(start_snapshot().cursors[cursor_index].index());
        document().delete_text_in_range(cursor.selection());

        if (!start_snapshot().cursors[cursor_index].selection().empty()) {
            do_insert(document(), cursors, cursor_index, selection_text(cursor_index));
        }
    }
}

DeleteCommand::DeleteCommand(Document& document) : DeltaBackedCommand(document) {}

DeleteCommand::~DeleteCommand() {}

bool DeleteCommand::do_execute(Display&, MultiCursor& cursors) {
    bool modified = false;
    for (int i = cursors.size() - 1; i >= 0; i--) {
        auto& cursor = cursors[i];
        if (!cursor.selection().empty()) {
            document().delete_text_in_range(cursor.selection());
            modified = true;
        }
    }

    return modified;
}

void DeleteCommand::do_undo(Display&, MultiCursor& cursors) {
    for (int i = cursors.size() - 1; i >= 0; i--) {
        auto& cursor = cursors[i];
        if (!start_snapshot().cursors[i].selection().empty()) {
            cursor.clear_selection();
            InsertCommand::do_insert(document(), cursors, i, selection_text(i));
        }
    }
}

SwapLinesCommand::SwapLinesCommand(Document& document, SwapDirection direction) : DeltaBackedCommand(document), m_direction(direction) {}

SwapLinesCommand::~SwapLinesCommand() {}

bool SwapLinesCommand::do_execute(Display&, MultiCursor& cursors) {
    cursors.remove_secondary_cursors();
    auto& cursor = cursors.main_cursor();
    bool ret = do_swap(cursor, m_direction);
    return ret;
}

bool SwapLinesCommand::do_swap(Cursor& cursor, SwapDirection direction) {
    auto selection_start = cursor.selection().start();
    auto selection_end = cursor.selection().end();

    if ((selection_start.line_index() == 0 && direction == SwapDirection::Up) ||
        (selection_end.line_index() == document().last_line_index() && direction == SwapDirection::Down)) {
        return false;
    }

    if (direction == SwapDirection::Up) {
        document().move_line_to(selection_start.line_index() - 1, selection_end.line_index());
    } else {
        document().move_line_to(selection_end.line_index() + 1, selection_start.line_index());
    }

    return true;
}

void SwapLinesCommand::do_undo(Display&, MultiCursor& cursors) {
    auto& cursor = cursors.main_cursor();
    do_swap(cursor, m_direction == SwapDirection::Up ? SwapDirection::Down : SwapDirection::Up);
}
}
