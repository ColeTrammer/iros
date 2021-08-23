#pragma once

#include <edit/character_metadata.h>
#include <edit/forward.h>
#include <edit/multicursor.h>
#include <edit/suggestions.h>
#include <eventloop/forward.h>
#include <graphics/color.h>
#include <graphics/forward.h>
#include <liim/forward.h>
#include <liim/function.h>
#include <liim/maybe.h>
#include <liim/pointers.h>

namespace Edit {
class Display {
public:
    virtual ~Display();

    virtual int rows() const = 0;
    virtual int cols() const = 0;

    int scroll_row_offset() const { return m_scroll_row_offset; }
    int scroll_col_offset() const { return m_scroll_col_offset; }

    void set_scroll_row_offset(int row_offset) { set_scroll_offsets(row_offset, m_scroll_col_offset); }
    void set_scroll_col_offset(int col_offset) { set_scroll_offsets(m_scroll_row_offset, col_offset); }
    void set_scroll_offsets(int row_offset, int col_offset);

    void scroll_up(int times) { scroll(-times, 0); }
    void scroll_down(int times) { scroll(times, 0); }
    void scroll_left(int times) { scroll(0, -times); }
    void scroll_right(int times) { scroll(0, times); }
    void scroll(int vertical, int horizontal);

    virtual App::Object& this_widget() = 0;
    virtual TextIndex text_index_at_mouse_position(const Point& point) = 0;
    virtual RenderedLine compose_line(const Line& line) = 0;
    virtual void output_line(int row, int col_offset, const StringView& text, const Vector<CharacterMetadata>& metadata) = 0;
    virtual void schedule_update() = 0;
    virtual int enter() = 0;
    virtual void send_status_message(String message) = 0;
    virtual void prompt(String, Function<void(Maybe<String>)> callback) { callback.safe_call({}); }
    virtual void enter_search(String starting_text) = 0;
    virtual void do_open_prompt() = 0;
    virtual void quit() = 0;

    virtual void do_compute_suggestions() {}
    virtual void show_suggestions_panel() {}
    virtual void hide_suggestions_panel() {}

    virtual void set_clipboard_contents(LIIM::String text, bool is_whole_line = false) = 0;
    virtual String clipboard_contents(bool& is_whole_line) const = 0;

    virtual void notify_line_count_changed();
    void notify_did_delete_lines(int line_index, int line_count);
    void notify_did_add_lines(int line_index, int line_count);
    void notify_did_split_line(int line_index, int index_into_line);
    void notify_did_merge_lines(int first_line_index, int first_line_length, int second_line_index);
    void notify_did_add_to_line(int line_index, int index_into_line, int bytes_added);
    void notify_did_delete_from_line(int line_index, int index_into_line, int bytes_deleted);
    void notify_did_move_line_to(int line, int destination);

    void set_document(SharedPtr<Document> document);

    Document* document() { return m_document.get(); }
    const Document* document() const { return m_document.get(); }

    SharedPtr<Document> document_as_shared() const { return m_document; }

    MultiCursor& cursors() { return m_cursors; }
    const MultiCursor& cursors() const { return m_cursors; }

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

protected:
    Display();

    virtual void document_did_change() {}
    virtual void suggestions_did_change(const Maybe<TextRange>&) {}

private:
    SharedPtr<Document> m_document;
    MultiCursor m_cursors;
    Vector<RenderedLine> m_rendered_lines;
    Suggestions m_suggestions;
    int m_scroll_row_offset { 0 };
    int m_scroll_col_offset { 0 };
};
}
