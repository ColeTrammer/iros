#include <edit/display.h>
#include <edit/document.h>
#include <edit/rendered_line.h>
#include <liim/string.h>
#include <liim/vector.h>

namespace Edit {
Display::Display() : m_cursors { *this } {}

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
    m_rendered_lines.resize(m_document->num_lines());
    invalidate_all_lines();
    hide_suggestions_panel();
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
    invalidate_all_line_rects();
}

void Display::scroll(int vertical, int horizontal) {
    auto row_scroll_max = max(0, document()->num_rendered_lines(*this) - rows());
    set_scroll_offsets(clamp(m_scroll_row_offset + vertical, 0, row_scroll_max), max(m_scroll_col_offset + horizontal, 0));
}

RenderedLine& Display::rendered_line_at_index(int index) {
    assert(index < m_rendered_lines.size());
    return m_rendered_lines[index];
}

void Display::invalidate_all_lines() {
    m_rendered_lines.clear();
    if (auto doc = document()) {
        m_rendered_lines.resize(doc->num_lines());
    }
    invalidate_all_line_rects();
}

void Display::invalidate_line(int line_index) {
    auto& info = rendered_line_at_index(line_index);

    invalidate_all_line_rects();

    info.rendered_lines.clear();
    info.position_ranges.clear();
}

void Display::toggle_show_line_numbers() {
    set_show_line_numbers(!m_show_line_numbers);
}

void Display::set_preview_auto_complete(bool b) {
    if (m_preview_auto_complete == b) {
        return;
    }

    m_preview_auto_complete = b;
    if (document()) {
        document()->invalidate_rendered_contents(cursors().main_cursor().referenced_line(*document()));
    }
}

void Display::set_show_line_numbers(bool b) {
    if (m_show_line_numbers == b) {
        return;
    }

    m_show_line_numbers = b;
    did_set_show_line_numbers();
}

void Display::set_word_wrap_enabled(bool b) {
    if (m_word_wrap_enabled == b) {
        return;
    }

    m_word_wrap_enabled = b;
    if (document()) {
        document()->invalidate_all_rendered_contents();
    }
}

void Display::install_document_listeners(Document& new_document) {
    new_document.on<DeleteLines>(this_widget(), [this](const DeleteLines& event) {
        for (int i = 0; i < event.line_count(); i++) {
            m_rendered_lines.remove(event.line_index());
        }
        invalidate_all_line_rects();
    });

    new_document.on<AddLines>(this_widget(), [this](const AddLines& event) {
        for (int i = 0; i < event.line_count(); i++) {
            m_rendered_lines.insert({}, event.line_index());
        }
        invalidate_all_line_rects();
    });

    new_document.on<SplitLines>(this_widget(), [this](const SplitLines& event) {
        m_rendered_lines.insert({}, event.line_index() + 1);
        document()->line_at_index(event.line_index()).invalidate_rendered_contents(*document(), *this);
        invalidate_all_line_rects();
    });

    new_document.on<MergeLines>(this_widget(), [this](const MergeLines& event) {
        m_rendered_lines.remove(event.second_line_index());
        document()->line_at_index(event.first_line_index()).invalidate_rendered_contents(*document(), *this);
        invalidate_all_line_rects();
    });

    new_document.on<AddToLine>(this_widget(), [this](const AddToLine& event) {
        document()->line_at_index(event.line_index()).invalidate_rendered_contents(*document(), *this);
    });

    new_document.on<DeleteFromLine>(this_widget(), [this](const DeleteFromLine& event) {
        document()->line_at_index(event.line_index()).invalidate_rendered_contents(*document(), *this);
    });

    new_document.on<MoveLineTo>(this_widget(), [this](const MoveLineTo& event) {
        auto line_min = min(event.line(), event.destination());
        auto line_max = max(event.line(), event.destination());

        if (event.line() > event.destination()) {
            m_rendered_lines.rotate_right(line_min, line_max + 1);
        } else {
            m_rendered_lines.rotate_left(line_min, line_max + 1);
        }

        invalidate_all_line_rects();
    });

    cursors().install_document_listeners(new_document);
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

Task<Maybe<String>> Display::prompt(String, String) {
    co_return {};
}

App::ObjectBoundCoroutine Display::do_open_prompt() {
    co_return;
}
}
