#include <edit/document.h>
#include <edit/panel.h>
#include <liim/string.h>
#include <liim/vector.h>

namespace Edit {
Panel::Panel() {}

Panel::~Panel() {}

void Panel::set_document(UniquePtr<Document> document) {
    if (m_document == document) {
        return;
    }

    m_document = move(document);
    m_cursors.remove_secondary_cursors();
    m_cursors.main_cursor().set({ 0, 0 });
    document_did_change();
}

UniquePtr<Document> Panel::take_document() {
    if (!m_document) {
        return nullptr;
    }

    auto document = move(m_document);
    document_did_change();
    return document;
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
