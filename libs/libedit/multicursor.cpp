#include <edit/display.h>
#include <edit/document.h>
#include <edit/multicursor.h>
#include <edit/position.h>
#include <edit/text_range_collection.h>

namespace Edit {
MultiCursor::MultiCursor() : m_cursors(Vector<Cursor>::create_from_single_element({})) {}

void MultiCursor::remove_duplicate_cursors() {
    for (int i = 0; i < m_cursors.size(); i++) {
        auto& current = m_cursors[i];

        // NOTE: for Selection::overlaps() to behave correctly, empty selections must be "positioned" at the cursor.
        if (current.selection().empty()) {
            current.selection().begin(current.index());
        }
        while (i + 1 < m_cursors.size()) {
            auto& next_cursor = m_cursors[i + 1];
            if (next_cursor.selection().empty()) {
                next_cursor.selection().begin(next_cursor.index());
            }

            if (current.selection().overlaps(next_cursor.selection())) {
                current.selection().merge(m_cursors[i + 1].selection());
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

void MultiCursor::add_cursor(Document& document, Display& display, AddCursorMode mode) {
    switch (mode) {
        case AddCursorMode::Up:
            m_cursors.insert(m_cursors.first(), 0);
            document.move_cursor_up(display, m_cursors.first());
            m_main_cursor_index++;
            break;
        case AddCursorMode::Down:
            m_cursors.add(m_cursors.last());
            document.move_cursor_down(display, m_cursors.last());
            break;
    }
}

void MultiCursor::add_cursor_at(Document& document, Display& display, const TextIndex& index, const Selection& selection) {
    int i = 0;
    for (; i < m_cursors.size(); i++) {
        // Duplicate cursors should not be created.
        if (m_cursors[i].index() == index) {
            return;
        }

        if (m_cursors[i].index() > index) {
            break;
        }
    }

    auto cursor = Cursor {};
    cursor.set(index);
    cursor.selection() = selection;
    cursor.compute_max_col(document, display);
    m_cursors.insert(move(cursor), i);
    if (i <= m_main_cursor_index) {
        m_main_cursor_index++;
    }
}

void MultiCursor::install_document_listeners(Display& display, Document& document) {
    document.on<DeleteLines>(display.this_widget(), [this, &display, &document](const DeleteLines& event) {
        for (auto& cursor : m_cursors) {
            if (cursor.selection().overlaps(Selection { { event.line_index(), 0 }, { event.line_index() + event.line_count(), 0 } })) {
                cursor.selection().clear();
            }
            if (cursor.line_index() < event.line_index()) {
                continue;
            }
            if (cursor.line_index() == event.line_index()) {
                if (event.line_index() == document.num_lines()) {
                    cursor.set({ document.num_lines() - 1, document.last_line().length() });
                    continue;
                }
                document.clear_selection(cursor);
                document.move_cursor_to(display, cursor,
                                        document.text_index_at_absolute_position(display, cursor.absolute_position(document, display)));
                continue;
            }
            cursor.move_up_preserving_selection(event.line_count());
        }
    });

    document.on<AddLines>(display.this_widget(), [this, &display, &document](const AddLines& event) {
        for (auto& cursor : m_cursors) {
            if (cursor.selection().overlaps(Selection { { event.line_index(), 0 }, { event.line_index() + event.line_count(), 0 } })) {
                cursor.selection().clear();
            }
            if (cursor.line_index() < event.line_index()) {
                continue;
            }
            cursor.move_down_preserving_selection(event.line_count());
        }
    });

    document.on<SplitLines>(display.this_widget(), [this, &display, &document](const SplitLines& event) {
        for (auto& cursor : m_cursors) {
            if (cursor.selection().overlaps(Selection { { event.line_index(), event.index_into_line() }, { event.line_index() + 1, 0 } })) {
                cursor.selection().clear();
            }
            if (cursor.line_index() != event.line_index() || cursor.index_into_line() < event.index_into_line()) {
                continue;
            }
            cursor.set({ event.line_index() + 1, cursor.index_into_line() - event.index_into_line() });
            cursor.compute_max_col(document, display);
        }
    });

    document.on<MergeLines>(display.this_widget(), [this, &display, &document](const MergeLines& event) {
        for (auto& cursor : m_cursors) {
            if (cursor.selection().overlaps(Selection { { event.second_line_index(), 0 }, { event.second_line_index() + 1, 0 } })) {
                cursor.selection().clear();
            }
            if (cursor.line_index() != event.second_line_index()) {
                continue;
            }
            cursor.set({ event.first_line_index(), event.first_line_length() + cursor.index_into_line() });
            cursor.compute_max_col(document, display);
        }
    });

    document.on<AddToLine>(display.this_widget(), [this, &display, &document](const AddToLine& event) {
        for (auto& cursor : m_cursors) {
            if (cursor.selection().overlaps(Selection { { event.line_index(), event.index_into_line() },
                                                        { event.line_index(), event.index_into_line() + event.bytes_added() } })) {
                cursor.selection().clear();
            }
            if (cursor.line_index() != event.line_index()) {
                continue;
            }
            if (cursor.index_into_line() < event.index_into_line()) {
                continue;
            }
            cursor.move_right_preserving_selection(event.bytes_added());
            cursor.compute_max_col(document, display);
        }
    });

    document.on<DeleteFromLine>(display.this_widget(), [this, &display, &document](const DeleteFromLine& event) {
        for (auto& cursor : m_cursors) {
            if (cursor.selection().overlaps(Selection { { event.line_index(), event.index_into_line() },
                                                        { event.line_index(), event.index_into_line() + event.bytes_deleted() } })) {
                cursor.selection().clear();
            }
            if (cursor.line_index() != event.line_index()) {
                continue;
            }
            if (cursor.index_into_line() <= event.index_into_line()) {
                continue;
            }
            cursor.move_left_preserving_selection(event.bytes_deleted());
            cursor.compute_max_col(document, display);
        }
    });

    document.on<MoveLineTo>(display.this_widget(), [this, &display, &document](const MoveLineTo& event) {
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

bool MultiCursor::should_show_auto_complete_text_at(const Display& display, const Document& document, const Line& line,
                                                    int index_into_line) const {
    return display.preview_auto_complete() && &main_cursor().referenced_line(document) == &line &&
           main_cursor().index_into_line() == index_into_line;
}

Maybe<String> MultiCursor::preview_auto_complete_text(Display& display) const {
    display.compute_suggestions();
    auto& suggestions = display.suggestions();
    if (suggestions.size() != 1) {
        return {};
    }

    auto& suggestion = suggestions.first();

    // Don't show any preview if the suggestion's prefix is not aligned (because of fuzzy matching).
    auto current_prefix = display.document()->text_in_range(suggestion.start(), display.cursors().main_cursor().index());
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
    auto current_text = display.document()->text_in_range(suggestion.start(), end);
    if (suggestion.content() == current_text.view()) {
        return {};
    }

    return String { suggestion.content().substring(current_prefix.size()) };
}

TextRangeCollection MultiCursor::cursor_text_ranges(const Document& document) const {
    auto collection = TextRangeCollection { document };
    for (auto& cursor : m_cursors) {
        auto start = cursor.index();
        auto end = TextIndex { cursor.line_index(), cursor.index_into_line() + 1 };
        auto metadata = CharacterMetadata { &cursor == &main_cursor() ? CharacterMetadata::Flags::MainCursor
                                                                      : CharacterMetadata::Flags::SecondaryCursor };
        collection.add(TextRange { start, end, metadata });
    }
    return collection;
}

TextRangeCollection MultiCursor::selections(const Document& document) const {
    auto collection = TextRangeCollection { document };
    for (auto& cursor : m_cursors) {
        collection.add(cursor.selection().text_range());
    }
    return collection;
}

MultiCursor::Snapshot MultiCursor::snapshot() const {
    return m_cursors;
}

void MultiCursor::restore(const Snapshot& snapshot) {
    m_cursors = snapshot;
}
}
