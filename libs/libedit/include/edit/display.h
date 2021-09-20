#pragma once

#include <edit/absolute_position.h>
#include <edit/character_metadata.h>
#include <edit/forward.h>
#include <edit/multicursor.h>
#include <edit/suggestions.h>
#include <edit/text_range_collection.h>
#include <eventloop/event.h>
#include <eventloop/forward.h>
#include <graphics/color.h>
#include <graphics/forward.h>
#include <liim/forward.h>
#include <liim/function.h>
#include <liim/maybe.h>
#include <liim/pointers.h>

APP_EVENT(Edit, SplitDisplayEvent, App::Event, (), (), ())
APP_EVENT(Edit, NewDisplayEvent, App::Event, (), (), ())

namespace Edit {
enum class AutoCompleteMode { Never, Always };

class Display {
public:
    virtual ~Display();

    virtual int rows() const = 0;
    virtual int cols() const = 0;

    AbsolutePosition scroll_offset() const { return m_scroll_offset; }
    void set_scroll_offset(const AbsolutePosition& offset);

    void scroll_up(int times) { scroll(-times, 0); }
    void scroll_down(int times) { scroll(times, 0); }
    void scroll_left(int times) { scroll(0, -times); }
    void scroll_right(int times) { scroll(0, times); }
    void scroll(int vertical, int horizontal);

    virtual App::Object& this_widget() = 0;
    virtual TextIndex text_index_at_mouse_position(const Point& point) = 0;
    virtual RenderedLine compose_line(const Line& line) = 0;
    virtual void output_line(int row, int col_offset, const RenderedLine& line, int line_index) = 0;
    virtual void invalidate_all_line_rects() = 0;
    virtual void invalidate_line_rect(int row_in_display) = 0;
    virtual int enter() = 0;
    virtual void send_status_message(String message) = 0;
    virtual Task<Maybe<String>> prompt(String message, String initial_value = "");
    virtual void enter_search(String starting_text) = 0;
    virtual App::ObjectBoundCoroutine do_open_prompt();
    virtual void quit() = 0;

    virtual void do_compute_suggestions() {}
    virtual void show_suggestions_panel() {}
    virtual void hide_suggestions_panel() {}

    virtual void set_clipboard_contents(LIIM::String text, bool is_whole_line = false) = 0;
    virtual String clipboard_contents(bool& is_whole_line) const = 0;

    void update_metadata(int line_index);
    void invalidate_metadata() { invalidate_all_line_rects(); }

    void set_document(SharedPtr<Document> document);

    Document* document() { return m_document.get(); }
    const Document* document() const { return m_document.get(); }

    SharedPtr<Document> document_as_shared() const { return m_document; }

    MultiCursor& cursors() { return m_cursors; }
    const MultiCursor& cursors() const { return m_cursors; }

    AutoCompleteMode auto_complete_mode() const { return m_auto_complete_mode; }
    void set_auto_complete_mode(AutoCompleteMode mode) { m_auto_complete_mode = mode; }

    bool preview_auto_complete() const { return m_preview_auto_complete; }
    void set_preview_auto_complete(bool b);

    bool show_line_numbers() const { return m_show_line_numbers; }
    void toggle_show_line_numbers();
    void set_show_line_numbers(bool b);

    bool word_wrap_enabled() const { return m_word_wrap_enabled; }
    void toggle_word_wrap_enabled();
    void set_word_wrap_enabled(bool b);

    Suggestions& suggestions() { return m_suggestions; }
    const Suggestions& suggestions() const { return m_suggestions; }

    void compute_suggestions();
    void set_suggestions(Vector<Suggestion> suggestions);

    struct RenderingInfo {
        Maybe<vga_color> fg;
        Maybe<vga_color> bg;
        bool bold { false };
        bool main_cursor { false };
        bool secondary_cursor { false };
    };

    RenderingInfo rendering_info_for_metadata(const CharacterMetadata& metadata) const;

    RenderedLine& rendered_line_at_index(int index);
    const RenderedLine& rendered_line_at_index(int index) const { return const_cast<Display&>(*this).rendered_line_at_index(index); }

    void invalidate_all_lines();
    void invalidate_line(int line_index);

    void clear_search();
    void update_search_results();
    void move_cursor_to_next_search_match();
    void select_next_word_at_cursor();
    void replace_next_search_match(const String& replacement);

    void set_search_text(String text);
    const TextRangeCollection& search_results() const { return m_search_results; }
    const String& search_text() const { return m_search_text; }

protected:
    Display();

    virtual void document_did_change() {}
    virtual void suggestions_did_change(const Maybe<TextRange>&) {}
    virtual void install_document_listeners(Document& document);
    virtual void did_set_show_line_numbers() {}

private:
    void uninstall_document_listeners(Document& document);

    SharedPtr<Document> m_document;
    Vector<RenderedLine> m_rendered_lines;

    MultiCursor m_cursors;

    Suggestions m_suggestions;
    AutoCompleteMode m_auto_complete_mode { AutoCompleteMode::Never };
    bool m_preview_auto_complete { false };

    String m_search_text;
    TextRangeCollection m_search_results;
    int m_search_result_index { 0 };

    bool m_word_wrap_enabled { true };
    bool m_show_line_numbers { false };

    AbsolutePosition m_scroll_offset;
};
}
