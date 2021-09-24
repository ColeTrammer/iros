#include <edit/absolute_position.h>
#include <edit/display.h>
#include <edit/document.h>
#include <edit/multicursor.h>
#include <edit/relative_position.h>
#include <edit/text_range_collection.h>

namespace Edit {
MultiCursor::MultiCursor(Display& display) : m_display { display }, m_cursors { Vector<Cursor>::create_from_single_element({}) } {}

void MultiCursor::remove_duplicate_cursors() {
    for (int i = 0; i < m_cursors.size(); i++) {
        auto& current = m_cursors[i];
        while (i + 1 < m_cursors.size()) {
            auto& next_cursor = m_cursors[i + 1];
            if (current.selection().overlaps(next_cursor.selection())) {
                current.merge_selections(m_cursors[i + 1]);
                m_cursors.remove(i + 1);
                if (m_main_cursor_index >= i + 1) {
                    m_main_cursor_index--;
                }
                continue;
            }
            break;
        }
    }
}

void MultiCursor::remove_secondary_cursors() {
    auto main_cursor_save = main_cursor();
    m_cursors.clear();
    m_cursors.add(main_cursor_save);
    m_main_cursor_index = 0;
}

Cursor& MultiCursor::main_cursor() {
    return m_cursors[m_main_cursor_index];
}

Cursor* MultiCursor::add_cursor(Document& document, AddCursorMode mode) {
    switch (mode) {
        case AddCursorMode::Up:
            m_cursors.insert(m_cursors.first(), 0);
            document.move_cursor_up(m_display, m_cursors.first());
            m_main_cursor_index++;
            return &m_cursors.first();
        case AddCursorMode::Down:
            m_cursors.add(m_cursors.last());
            document.move_cursor_down(m_display, m_cursors.last());
            return &m_cursors.last();
    }
    return nullptr;
}

Cursor* MultiCursor::add_cursor_at(Document&, const TextIndex& index, const TextIndex& selection_start) {
    int i = 0;
    for (; i < m_cursors.size(); i++) {
        // Duplicate cursors should not be created.
        if (m_cursors[i].index() == index) {
            return nullptr;
        }

        if (m_cursors[i].index() > index) {
            break;
        }
    }

    auto cursor = Cursor {};
    cursor.set_selection_anchor(selection_start);
    cursor.set(index);
    cursor.compute_max_col(m_display);
    m_cursors.insert(move(cursor), i);
    if (i <= m_main_cursor_index) {
        m_main_cursor_index++;
    }
    return &m_cursors[i];
}

void MultiCursor::install_document_listeners(Document& document) {
    document.on<DeleteLines>(m_display.this_widget(), [this, &document](const DeleteLines& event) {
        invalidate_cursor_history();

        for (auto& cursor : m_cursors) {
            if (cursor.selection().overlaps({ { event.line_index(), 0 }, { event.line_index() + event.line_count(), 0 } })) {
                cursor.clear_selection();
            }
            if (cursor.line_index() < event.line_index()) {
                continue;
            }
            if (cursor.line_index() == event.line_index()) {
                if (event.line_index() == document.line_count()) {
                    cursor.set({ document.last_line_index(), document.last_line().length() });
                    continue;
                }
                cursor.clear_selection();
                document.move_cursor_to(m_display, cursor, m_display.text_index_at_absolute_position(cursor.absolute_position(m_display)));
                continue;
            }
            cursor.move_up_preserving_selection(event.line_count());
        }
    });

    document.on<AddLines>(m_display.this_widget(), [this](const AddLines& event) {
        invalidate_cursor_history();

        for (auto& cursor : m_cursors) {
            if (cursor.selection().overlaps({ { event.line_index(), 0 }, { event.line_index() + event.line_count(), 0 } })) {
                cursor.clear_selection();
            }
            if (cursor.line_index() < event.line_index()) {
                continue;
            }
            cursor.move_down_preserving_selection(event.line_count());
        }
    });

    document.on<SplitLines>(m_display.this_widget(), [this](const SplitLines& event) {
        invalidate_cursor_history();

        for (auto& cursor : m_cursors) {
            if (cursor.selection().overlaps({ { event.line_index(), event.index_into_line() }, { event.line_index() + 1, 0 } })) {
                cursor.clear_selection();
            }
            if (cursor.line_index() != event.line_index() || cursor.index_into_line() < event.index_into_line()) {
                continue;
            }
            cursor.set({ event.line_index() + 1, cursor.index_into_line() - event.index_into_line() });
            cursor.compute_max_col(m_display);
        }
    });

    document.on<MergeLines>(m_display.this_widget(), [this](const MergeLines& event) {
        invalidate_cursor_history();

        for (auto& cursor : m_cursors) {
            if (cursor.selection().overlaps({ { event.second_line_index(), 0 }, { event.second_line_index() + 1, 0 } })) {
                cursor.clear_selection();
            }
            if (cursor.line_index() != event.second_line_index()) {
                continue;
            }
            cursor.set({ event.first_line_index(), event.first_line_length() + cursor.index_into_line() });
            cursor.compute_max_col(m_display);
        }
    });

    document.on<AddToLine>(m_display.this_widget(), [this](const AddToLine& event) {
        invalidate_cursor_history();

        for (auto& cursor : m_cursors) {
            if (cursor.selection().overlaps({ { event.line_index(), event.index_into_line() },
                                              { event.line_index(), event.index_into_line() + event.bytes_added() } })) {
                cursor.clear_selection();
            }
            if (cursor.line_index() != event.line_index()) {
                continue;
            }
            if (cursor.index_into_line() < event.index_into_line()) {
                continue;
            }
            cursor.move_right_preserving_selection(event.bytes_added());
            cursor.compute_max_col(m_display);
        }
    });

    document.on<DeleteFromLine>(m_display.this_widget(), [this](const DeleteFromLine& event) {
        invalidate_cursor_history();

        for (auto& cursor : m_cursors) {
            if (cursor.selection().overlaps({ { event.line_index(), event.index_into_line() },
                                              { event.line_index(), event.index_into_line() + event.bytes_deleted() } })) {
                cursor.clear_selection();
            }
            if (cursor.line_index() != event.line_index()) {
                continue;
            }
            if (cursor.index_into_line() <= event.index_into_line()) {
                continue;
            }
            cursor.move_left_preserving_selection(event.bytes_deleted());
            cursor.compute_max_col(m_display);
        }
    });

    document.on<MoveLineTo>(m_display.this_widget(), [this](const MoveLineTo& event) {
        invalidate_cursor_history();

        auto line_min = min(event.line(), event.destination());
        auto line_max = max(event.line(), event.destination());
        for (auto& cursor : m_cursors) {
            if (cursor.line_index() < line_min || cursor.line_index() > line_max) {
                continue;
            }

            if (cursor.line_index() == event.line()) {
                cursor.move_down_preserving_selection(event.line() - event.destination());
                continue;
            }

            if (event.line() > event.destination()) {
                cursor.move_down_preserving_selection(1);
            } else {
                cursor.move_up_preserving_selection(1);
            }
        }
    });
}

bool MultiCursor::should_show_auto_complete_text_at(const Document& document, const Line& line, int index_into_line) const {
    return m_display.preview_auto_complete() && &main_cursor().referenced_line(document) == &line &&
           main_cursor().index_into_line() == index_into_line;
}

Maybe<String> MultiCursor::preview_auto_complete_text() const {
    m_display.compute_suggestions();
    auto& suggestions = m_display.suggestions();
    if (suggestions.size() != 1) {
        return {};
    }

    auto& suggestion = suggestions.first();

    // Don't show any preview if the suggestion's prefix is not aligned (because of fuzzy matching).
    auto current_prefix = m_display.document()->text_in_range({ suggestion.start(), m_display.main_cursor().index() });
    if (!suggestion.content().starts_with(current_prefix.view())) {
        return {};
    }

    // Don't show any preview if the suggestion does not actually change the document.
    auto end = suggestion.start();
    for (size_t i = 0; i < suggestion.content().size(); i++) {
        if (suggestion.content()[i] == '\n') {
            end.set(end.line_index() + 1, 0);
        } else {
            end.set_index_into_line(end.index_into_line() + 1);
        }
    }
    if (end.index_into_line() > m_display.document()->line_at_index(end.line_index()).length()) {
        end.set_index_into_line(m_display.document()->line_at_index(end.line_index()).length());
    }

    auto current_text = m_display.document()->text_in_range({ suggestion.start(), end });
    if (suggestion.content().view() == current_text.view()) {
        return {};
    }

    return String { suggestion.content().substring(current_prefix.size()) };
}

TextRangeCollection MultiCursor::cursor_text_ranges() const {
    auto collection = TextRangeCollection {};
    for (auto& cursor : m_cursors) {
        auto start = cursor.index();
        auto end = TextIndex { cursor.line_index(), cursor.index_into_line() + 1 };
        auto metadata = CharacterMetadata { &cursor == &main_cursor() ? CharacterMetadata::Flags::MainCursor
                                                                      : CharacterMetadata::Flags::SecondaryCursor };
        collection.add(TextRange { start, end, metadata });
    }
    return collection;
}

TextRangeCollection MultiCursor::selections() const {
    auto collection = TextRangeCollection {};
    for (auto& cursor : m_cursors) {
        collection.add(cursor.selection());
    }
    return collection;
}

MultiCursor::Snapshot MultiCursor::snapshot() const {
    return { m_cursors, m_main_cursor_index };
}

void MultiCursor::restore(Document& document, const Snapshot& snapshot) {
    cursor_save();
    m_cursors = snapshot.cursors;
    m_main_cursor_index = snapshot.main_cursor_index;
    invalidate_based_on_last_snapshot(document);
    m_history.remove_last();
}

void MultiCursor::invalidate_based_on_last_snapshot(Document&) {
    // FIXME: this could be made a lot more precise than invalidating the entire document's metadata.
    m_display.invalidate_metadata();
}

void MultiCursor::invalidate_cursor_history() {
    m_history.clear();
}

void MultiCursor::cursor_save() {
    auto state = snapshot();
    if (!m_history.empty() && m_history.last() == state) {
        return;
    }

    m_history.add(move(state));
}

void MultiCursor::cursor_undo(Document& document) {
    if (m_history.empty()) {
        return;
    }
    restore(document, m_history.last());
    m_history.remove_last();
}
}
