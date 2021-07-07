#pragma once

#include <edit/character_metadata.h>
#include <edit/cursor.h>
#include <edit/forward.h>
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

    virtual void clear() = 0;
    virtual void set_text_at(int row, int col, char c, CharacterMetadata metadata) = 0;
    virtual void flush() = 0;
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

    virtual String inject_inline_text_for_line_index(int line_index) const;

    virtual void notify_line_count_changed() {}
    virtual void notify_now_is_a_good_time_to_draw_cursor() {}

    void set_document(UniquePtr<Document> document);
    UniquePtr<Document> take_document();

    Document* document() { return m_document.get(); }
    const Document* document() const { return m_document.get(); }

    Cursor& cursor() { return m_cursor; }
    const Cursor& cursor() const { return m_cursor; }

    struct RenderingInfo {
        Maybe<vga_color> fg;
        Maybe<vga_color> bg;
        bool bold { false };
    };

    RenderingInfo rendering_info_for_metadata(const CharacterMetadata& metadata) const;

protected:
    Panel();

    virtual void document_did_change() {}

private:
    UniquePtr<Document> m_document;
    Cursor m_cursor;
};
}
