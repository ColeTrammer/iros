#include <edit/document.h>
#include <edit/panel.h>
#include <edit/rendered_line.h>
#include <liim/string.h>
#include <liim/vector.h>

namespace Edit {
Panel::Panel() {}

Panel::~Panel() {}

void Panel::set_document(SharedPtr<Document> document) {
    if (m_document == document) {
        return;
    }

    m_document = move(document);
    m_cursors.remove_secondary_cursors();
    m_cursors.main_cursor().set({ 0, 0 });
    m_rendered_lines.clear();
    notify_line_count_changed();
    document_did_change();
}

Suggestions Panel::get_suggestions() const {
    return {};
}

void Panel::set_scroll_offsets(int row_offset, int col_offset) {
    if (m_scroll_row_offset == row_offset && m_scroll_col_offset == col_offset) {
        return;
    }

    m_scroll_row_offset = row_offset;
    m_scroll_col_offset = col_offset;
    schedule_update();
}

void Panel::scroll(int vertical, int horizontal) {
    auto row_scroll_max = document()->num_rendered_lines() - rows();
    set_scroll_offsets(clamp(m_scroll_row_offset + vertical, 0, row_scroll_max), max(m_scroll_col_offset + horizontal, 0));
}

RenderedLine& Panel::rendered_line_at_index(int index) {
    assert(index < m_rendered_lines.size());
    return m_rendered_lines[index];
}

void Panel::notify_line_count_changed() {
    if (!document()) {
        m_rendered_lines.clear();
        return;
    }
    m_rendered_lines.resize(document()->num_lines());
}

void Panel::notify_did_delete_lines(int line_index, int line_count) {
    for (int i = 0; i < line_count; i++) {
        m_rendered_lines.remove(line_index);
    }
    m_cursors.did_delete_lines(*document(), *this, line_index, line_count);
    notify_line_count_changed();
    schedule_update();
}

void Panel::notify_did_add_lines(int line_index, int line_count) {
    for (int i = 0; i < line_count; i++) {
        m_rendered_lines.insert({}, line_index);
    }
    m_cursors.did_add_lines(*document(), *this, line_index, line_count);
    notify_line_count_changed();
    schedule_update();
}

void Panel::notify_did_split_line(int line_index, int index_into_line) {
    m_rendered_lines.insert({}, line_index + 1);
    document()->line_at_index(line_index).invalidate_rendered_contents(*document(), *this);
    m_cursors.did_split_line(*document(), *this, line_index, index_into_line);
    notify_line_count_changed();
    schedule_update();
}

void Panel::notify_did_merge_lines(int first_line_index, int first_line_length, int second_line_index) {
    m_rendered_lines.remove(second_line_index);
    document()->line_at_index(first_line_index).invalidate_rendered_contents(*document(), *this);
    m_cursors.did_merge_lines(*document(), *this, first_line_index, first_line_length, second_line_index);
    notify_line_count_changed();
    schedule_update();
}

void Panel::notify_did_add_to_line(int line_index, int index_into_line, int bytes_added) {
    document()->line_at_index(line_index).invalidate_rendered_contents(*document(), *this);
    m_cursors.did_add_to_line(*document(), *this, line_index, index_into_line, bytes_added);
    schedule_update();
}

void Panel::notify_did_delete_from_line(int line_index, int index_into_line, int bytes_deleted) {
    document()->line_at_index(line_index).invalidate_rendered_contents(*document(), *this);
    m_cursors.did_delete_from_line(*document(), *this, line_index, index_into_line, bytes_deleted);
    schedule_update();
}

Panel::RenderingInfo Panel::rendering_info_for_metadata(const CharacterMetadata& metadata) const {
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
