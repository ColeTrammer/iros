#pragma once

#include <edit/display.h>
#include <eventloop/event.h>
#include <liim/hash_map.h>
#include <liim/maybe.h>
#include <liim/variant.h>
#include <liim/vector.h>
#include <time.h>
#include <tinput/terminal_input_parser.h>

namespace TInput {

class ReplDisplay final : public Edit::Display {
public:
    ReplDisplay(Repl& repl);
    virtual ~ReplDisplay() override;

    virtual int rows() const override { return m_rows; }
    virtual int cols() const override { return m_cols; };

    virtual Edit::RenderedLine compose_line(const Edit::Line& line) const override;
    virtual void output_line(int row, int col_offset, const StringView& text, const Vector<Edit::CharacterMetadata>& metadata) override;
    virtual void schedule_update() override { m_render_scheduled = true; }
    virtual int enter() override;
    virtual void send_status_message(String message) override;
    virtual Maybe<String> prompt(const String& message) override;
    virtual void enter_search(String starting_text) override;
    virtual void quit() override;

    virtual void set_clipboard_contents(String text, bool is_whole_line) override;
    virtual String clipboard_contents(bool& is_whole_line) const override;

    virtual Edit::Suggestions get_suggestions() const override;
    virtual void handle_suggestions(const Edit::Suggestions& suggestions) override;

    void set_coordinates(int rows, int cols);
    bool quit_by_interrupt() const { return m_quit_by_interrupt; }
    bool quit_by_eof() const { return m_quit_by_eof; }

    virtual void do_open_prompt() override;

private:
    virtual void document_did_change() override;

    void draw_cursor();

    String string_for_metadata(Edit::CharacterMetadata metadata) const;

    void print_char(char c, Edit::CharacterMetadata metadata);
    void flush_row(int line);
    void flush();
    void flush_if_needed();

    void set_quit_by_interrupt() { m_quit_by_interrupt = true; }
    void set_quit_by_eof() { m_quit_by_eof = true; }

    Maybe<String> enter_prompt(const String& message, String starting_text = "");

    int index(int row, int col) const;

    Vector<SharedPtr<Edit::Document>>& ensure_history_documents();
    void put_history_document(SharedPtr<Edit::Document> document, int history_index);
    SharedPtr<Edit::Document> history_document(int history_index);

    void move_history_up();
    void move_history_down();

    void get_absolute_row_position();

    Repl& m_repl;
    String m_main_prompt;
    String m_secondary_prompt;
    Vector<SharedPtr<Edit::Document>> m_history_documents;
    int m_history_index { -1 };

    mutable String m_prev_clipboard_contents;
    mutable bool m_prev_clipboard_contents_were_whole_line { false };
    TInput::TerminalInputParser m_input_parser;
    Edit::CharacterMetadata m_last_metadata_rendered;
    int m_rows { 0 };
    int m_cols { 0 };
    int m_last_rendered_height { 1 };
    int m_absolute_row_position { -1 };
    int m_visible_cursor_row { 0 };
    int m_visible_cursor_col { 0 };
    int m_exit_code { 0 };
    int m_consecutive_tabs { 0 };
    bool m_should_exit { false };
    bool m_quit_by_interrupt { false };
    bool m_quit_by_eof { false };
    bool m_render_scheduled { false };
};
}
