#pragma once

#include <edit/display_bridge_interface.h>
#include <edit/forward.h>
#include <eventloop/forward.h>
#include <liim/forward.h>

namespace Edit {
class DisplayBridge {
public:
    virtual ~DisplayBridge() {}

    // os_2 reflect begin
    virtual int rows() const = 0;
    virtual int cols() const = 0;

    virtual Edit::TextIndex text_index_at_mouse_position(const Point& point) = 0;
    virtual Edit::RenderedLine compose_line(const Edit::Line& line) = 0;
    virtual void output_line(int row, int col_offset, const Edit::RenderedLine& line, int line_index) = 0;
    virtual void invalidate_all_line_rects() = 0;
    virtual void invalidate_line_rect(int row_in_display) = 0;
    virtual int enter() = 0;
    virtual void send_status_message(String message) = 0;
    virtual Task<Maybe<String>> prompt(String message, String initial_value);
    virtual void enter_search(String starting_text) = 0;

    virtual void do_compute_suggestions() {}
    virtual void show_suggestions_panel() {}
    virtual void hide_suggestions_panel() {}

    virtual App::ObjectBoundCoroutine do_open_prompt() = 0;
    virtual App::ObjectBoundCoroutine quit() = 0;

    virtual void set_clipboard_contents(LIIM::String text, bool is_whole_line) = 0;
    virtual String clipboard_contents(bool& is_whole_line) const = 0;
    // os_2 reflect end

    // FIXME: these should be proper events
    virtual void document_did_change() {}
    virtual void suggestions_did_change(const Maybe<TextRange>&) {}
    virtual void did_set_show_line_numbers() {}

    virtual void install_document_listeners(Document&) {}
    virtual void uninstall_document_listeners(Document&) {}
};
}
