#pragma once

#include <edit/character_metadata.h>
#include <edit/forward.h>
#include <edit/multicursor.h>
#include <graphics/color.h>
#include <liim/forward.h>
#include <liim/maybe.h>
#include <liim/pointers.h>

namespace Edit {
class Panel {
public:
    virtual ~Panel();

    virtual int rows() const = 0;
    virtual int cols() const = 0;

    virtual RenderedLine compose_line(const Line& line) const = 0;
    virtual void output_line(int row, int col_offset, const StringView& text, const Vector<CharacterMetadata>& metadata) = 0;
    virtual void schedule_update() = 0;
    virtual int enter() = 0;
    virtual void send_status_message(String message) = 0;
    virtual Maybe<String> prompt(const String& message) = 0;
    virtual void enter_search(String starting_text) = 0;
    virtual void do_open_prompt() = 0;
    virtual void quit() = 0;

    virtual void set_clipboard_contents(LIIM::String text, bool is_whole_line = false) = 0;
    virtual String clipboard_contents(bool& is_whole_line) const = 0;

    virtual Suggestions get_suggestions() const;
    virtual void handle_suggestions(const Suggestions&) {}

    virtual void notify_line_count_changed() {}

    void set_document(UniquePtr<Document> document);
    UniquePtr<Document> take_document();

    Document* document() { return m_document.get(); }
    const Document* document() const { return m_document.get(); }

    MultiCursor& cursors() { return m_cursors; }
    const MultiCursor& cursors() const { return m_cursors; }

    struct RenderingInfo {
        Maybe<vga_color> fg;
        Maybe<vga_color> bg;
        bool bold { false };
        bool main_cursor { false };
        bool secondary_cursor { false };
    };

    RenderingInfo rendering_info_for_metadata(const CharacterMetadata& metadata) const;

protected:
    Panel();

    virtual void document_did_change() {}

private:
    UniquePtr<Document> m_document;
    MultiCursor m_cursors;
};
}
