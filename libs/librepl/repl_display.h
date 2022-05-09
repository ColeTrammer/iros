#pragma once

#include <edit/display.h>
#include <edit/display_bridge.h>
#include <eventloop/event.h>
#include <liim/hash_map.h>
#include <liim/option.h>
#include <liim/variant.h>
#include <liim/vector.h>
#include <repl/repl_base.h>
#include <time.h>
#include <tinput/terminal_input_parser.h>
#include <tui/panel.h>

#include "suggestions_panel.h"

namespace Repl {

class ReplDisplay final
    : public TUI::Panel
    , public Edit::DisplayBridge {
    APP_WIDGET_BASE(Edit::Display, TUI::Panel, ReplDisplay, self, self)

    EDIT_DISPLAY_INTERFACE_FORWARD(base())

public:
    explicit ReplDisplay(ReplBase& repl);
    virtual void did_attach() override;
    virtual ~ReplDisplay() override;

    // ^TUI::Panel
    virtual Option<Point> cursor_position() override;
    virtual void render() override;

    // ^Edit::Display
    virtual int rows() const override { return sized_rect().height(); }
    virtual int cols() const override { return sized_rect().width(); }

    virtual Edit::TextIndex text_index_at_mouse_position(const Point& point) override;
    virtual Edit::RenderedLine compose_line(const Edit::Line& line) override;
    virtual void output_line(int row, int col_offset, const Edit::RenderedLine& line, int line_index) override;
    virtual void invalidate_all_line_rects() override { invalidate(); }
    virtual void invalidate_line_rect(int row_in_display) override { invalidate(sized_rect().with_y(row_in_display).with_height(1)); }
    virtual int enter() override;
    virtual void send_status_message(String message) override;
    virtual void enter_search(String starting_text) override;

    virtual Task<Option<String>> prompt(String message, String initial_value) override;
    virtual App::ObjectBoundCoroutine do_open_prompt() override;
    virtual App::ObjectBoundCoroutine quit() override;

    virtual void set_clipboard_contents(String text, bool is_whole_line) override;
    virtual String clipboard_contents(bool& is_whole_line) const override;

    virtual void do_compute_suggestions() override;
    virtual void show_suggestions_panel() override;
    virtual void hide_suggestions_panel() override;

    bool quit_by_interrupt() const { return m_quit_by_interrupt; }
    bool quit_by_eof() const { return m_quit_by_eof; }

    void complete_suggestion(const Edit::MatchedSuggestion& suggestion);

private:
    virtual void document_did_change() override;
    virtual void suggestions_did_change(const Option<Edit::TextRange>& old_text_range) override;

    void move_up_rows(int count);

    void set_quit_by_interrupt() { m_quit_by_interrupt = true; }
    void set_quit_by_eof() { m_quit_by_eof = true; }

    Vector<SharedPtr<Edit::Document>>& ensure_history_documents();
    void put_history_document(SharedPtr<Edit::Document> document, int history_index);
    SharedPtr<Edit::Document> history_document(int history_index);

    void move_history_up();
    void move_history_down();

    ReplBase& m_repl;
    String m_main_prompt;
    String m_secondary_prompt;
    Vector<SharedPtr<Edit::Document>> m_history_documents;
    int m_history_index { -1 };

    SharedPtr<SuggestionsPanel> m_suggestions_panel;

    mutable String m_prev_clipboard_contents;
    mutable bool m_prev_clipboard_contents_were_whole_line { false };
    int m_last_rendered_row { 0 };
    int m_consecutive_tabs { 0 };
    bool m_quit_by_interrupt { false };
    bool m_quit_by_eof { false };
    bool m_suggest_based_on_history { false };
};
}
