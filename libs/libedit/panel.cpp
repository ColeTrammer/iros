#include <edit/document.h>
#include <edit/panel.h>

Panel::Panel() {}

Panel::~Panel() {}

void Panel::set_document(UniquePtr<Document> document) {
    if (m_document == document) {
        return;
    }

    m_document = move(document);
    document_did_change();
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

    if (metadata.highlighted() && metadata.selected()) {
        info.fg = VGA_COLOR_YELLOW;
        info.bg = VGA_COLOR_DARK_GREY;
    }

    return info;
}
