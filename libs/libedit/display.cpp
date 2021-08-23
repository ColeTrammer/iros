#include <edit/display.h>
#include <edit/document.h>
#include <edit/rendered_line.h>
#include <liim/string.h>
#include <liim/vector.h>

namespace Edit {
Display::Display() {}

Display::~Display() {}

void Display::set_document(SharedPtr<Document> document) {
    if (m_document == document) {
        return;
    }

    if (m_document) {
        uninstall_document_listeners(*m_document);
        m_document->unregister_display(*this);
    }
    m_document = move(document);
    if (m_document) {
        install_document_listeners(*m_document);
        m_document->register_display(*this);
    }

    m_cursors.remove_secondary_cursors();
    m_cursors.main_cursor().set({ 0, 0 });
    m_rendered_lines.clear();
    hide_suggestions_panel();
    notify_line_count_changed();
    document_did_change();
}

void Display::set_suggestions(Vector<Suggestion> suggestions) {
    auto old_suggestion_range = m_suggestions.current_text_range();
    m_suggestions.set_suggestions(move(suggestions));

    auto cursor_index = cursors().main_cursor().index();
    auto index_for_suggestion = TextIndex { cursor_index.line_index(), max(cursor_index.index_into_line() - 1, 0) };
    m_suggestions.set_current_text_range(document()->syntax_highlighting_info().range_at_text_index(index_for_suggestion));

    m_suggestions.compute_matches(*document(), cursors().main_cursor());

    suggestions_did_change(old_suggestion_range);
}

void Display::compute_suggestions() {
    document()->update_syntax_highlighting();
    do_compute_suggestions();
}

void Display::set_scroll_offsets(int row_offset, int col_offset) {
    if (m_scroll_row_offset == row_offset && m_scroll_col_offset == col_offset) {
        return;
    }

    m_scroll_row_offset = row_offset;
    m_scroll_col_offset = col_offset;
    schedule_update();
}

void Display::scroll(int vertical, int horizontal) {
    auto row_scroll_max = document()->num_rendered_lines(*this) - rows();
    set_scroll_offsets(clamp(m_scroll_row_offset + vertical, 0, row_scroll_max), max(m_scroll_col_offset + horizontal, 0));
}

RenderedLine& Display::rendered_line_at_index(int index) {
    assert(index < m_rendered_lines.size());
    return m_rendered_lines[index];
}

void Display::notify_line_count_changed() {
    if (!document()) {
        m_rendered_lines.clear();
        return;
    }
    m_rendered_lines.resize(document()->num_lines());
}

void Display::install_document_listeners(Document& new_document) {
    new_document.on<DeleteLines>(this_widget(), [this](const DeleteLines& event) {
        for (int i = 0; i < event.line_count(); i++) {
            m_rendered_lines.remove(event.line_index());
        }
        m_cursors.did_delete_lines(*document(), *this, event.line_index(), event.line_count());
        notify_line_count_changed();
        schedule_update();
    });

    new_document.on<AddLines>(this_widget(), [this](const AddLines& event) {
        for (int i = 0; i < event.line_count(); i++) {
            m_rendered_lines.insert({}, event.line_index());
        }
        m_cursors.did_add_lines(*document(), *this, event.line_index(), event.line_count());
        notify_line_count_changed();
        schedule_update();
    });

    new_document.on<SplitLines>(this_widget(), [this](const SplitLines& event) {
        m_rendered_lines.insert({}, event.line_index() + 1);
        document()->line_at_index(event.line_index()).invalidate_rendered_contents(*document(), *this);
        m_cursors.did_split_line(*document(), *this, event.line_index(), event.index_into_line());
        notify_line_count_changed();
        schedule_update();
    });

    new_document.on<MergeLines>(this_widget(), [this](const MergeLines& event) {
        m_rendered_lines.remove(event.second_line_index());
        document()->line_at_index(event.first_line_index()).invalidate_rendered_contents(*document(), *this);
        m_cursors.did_merge_lines(*document(), *this, event.first_line_index(), event.first_line_length(), event.second_line_index());
        notify_line_count_changed();
        schedule_update();
    });

    new_document.on<AddToLine>(this_widget(), [this](const AddToLine& event) {
        document()->line_at_index(event.line_index()).invalidate_rendered_contents(*document(), *this);
        m_cursors.did_add_to_line(*document(), *this, event.line_index(), event.index_into_line(), event.bytes_added());
        schedule_update();
    });

    new_document.on<DeleteFromLine>(this_widget(), [this](const DeleteFromLine& event) {
        document()->line_at_index(event.line_index()).invalidate_rendered_contents(*document(), *this);
        m_cursors.did_delete_from_line(*document(), *this, event.line_index(), event.index_into_line(), event.bytes_deleted());
        schedule_update();
    });

    new_document.on<MoveLineTo>(this_widget(), [this](const MoveLineTo& event) {
        auto line_min = min(event.line(), event.destination());
        auto line_max = max(event.line(), event.destination());
        auto& start_line = document()->line_at_index(line_min);
        auto& end_line = document()->line_at_index(line_max);

        auto start_line_size = start_line.rendered_line_count(*document(), *this);
        auto end_line_size = end_line.rendered_line_count(*document(), *this);

        auto rendered_line_min = start_line.absolute_row_position(*document(), *this);
        auto rendered_line_max = end_line.absolute_row_position(*document(), *this) + end_line_size;

        // FIXME: add Vector<T>::rotate() to replace this computational inefficent and ugly approach.
        if (event.line() > event.destination()) {
            for (int i = 0; i < start_line_size; i++) {
                m_rendered_lines.rotate_left(rendered_line_min, rendered_line_max);
            }
        } else {
            for (int i = 0; i < end_line_size; i++) {
                m_rendered_lines.rotate_right(rendered_line_min, rendered_line_max);
            }
        }

        document()->invalidate_all_rendered_contents();
        m_cursors.did_move_line_to(*document(), *this, event.line(), event.destination());
        schedule_update();
    });
}

void Display::uninstall_document_listeners(Document& document) {
    document.remove_listener(this_widget());
}

Display::RenderingInfo Display::rendering_info_for_metadata(const CharacterMetadata& metadata) const {
    RenderingInfo info;
    if (metadata.syntax_highlighting() & CharacterMetadata::Flags::SyntaxComment) {
        info.fg = VGA_COLOR_DARK_GREY;
    }

    if (metadata.highlighted()) {
        info.fg = VGA_COLOR_BLACK;
        info.bg = VGA_COLOR_YELLOW;
    }

    if (metadata.selected()) {
        info.fg = {};
        info.bg = VGA_COLOR_DARK_GREY;
    }

    if (metadata.syntax_highlighting() & CharacterMetadata::Flags::SyntaxOperator) {
        info.fg = VGA_COLOR_CYAN;
    }

    if (metadata.syntax_highlighting() & CharacterMetadata::Flags::SyntaxKeyword) {
        info.fg = VGA_COLOR_MAGENTA;
    }

    if (metadata.syntax_highlighting() & CharacterMetadata::Flags::SyntaxNumber) {
        info.fg = VGA_COLOR_RED;
    }

    if (metadata.syntax_highlighting() & CharacterMetadata::Flags::SyntaxIdentifier) {
        info.fg = VGA_COLOR_YELLOW;
        info.bold = true;
    }

    if (metadata.syntax_highlighting() & CharacterMetadata::Flags::SyntaxString) {
        info.fg = VGA_COLOR_GREEN;
    }

    if (metadata.syntax_highlighting() & CharacterMetadata::Flags::SyntaxImportant) {
        info.bold = true;
    }

    if (metadata.highlighted() && metadata.selected()) {
        info.fg = VGA_COLOR_YELLOW;
        info.bg = VGA_COLOR_DARK_GREY;
    }

    if (metadata.auto_complete_preview()) {
        info.fg = VGA_COLOR_DARK_GREY;
    }

    if (metadata.main_cursor()) {
        info.main_cursor = true;
    }

    if (metadata.secondary_cursor()) {
        info.secondary_cursor = true;
    }

    return info;
}
}
