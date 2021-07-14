#pragma once

#include <edit/panel.h>
#include <eventloop/event.h>
#include <eventloop/mouse_press_tracker.h>
#include <liim/hash_map.h>
#include <liim/maybe.h>
#include <liim/variant.h>
#include <liim/vector.h>
#include <time.h>

class TerminalPanel final : public Edit::Panel {
public:
    TerminalPanel();
    virtual ~TerminalPanel() override;

    virtual int rows() const override { return m_rows; }
    virtual int cols() const override;

    virtual void clear() override;
    virtual Edit::RenderedLine compose_line(const Edit::Line& line) const override;
    virtual void set_text_at(int row, int col, char c, Edit::CharacterMetadata metadata) override;
    virtual void flush() override;
    virtual int enter() override;
    virtual void send_status_message(String message) override;
    virtual Maybe<String> prompt(const String& message) override;
    virtual void enter_search(String starting_text) override;
    virtual void notify_line_count_changed() override;
    virtual void quit() override;

    virtual void set_clipboard_contents(String text, bool is_whole_line) override;
    virtual String clipboard_contents(bool& is_whole_line) const override;

    void set_coordinates(int row_offset, int col_offset, int rows, int cols);

    int col_offset() const { return m_col_offset; }
    int row_offset() const { return m_row_offset; }

    virtual void do_open_prompt() override;
    virtual void notify_now_is_a_good_time_to_draw_cursor() override;

private:
    struct Info {
        char ch;
        Edit::CharacterMetadata metadata;
    };

    TerminalPanel(int rows, int cols, int row_off, int col_off);

    virtual void document_did_change() override;

    Vector<Variant<Edit::KeyPress, App::MouseEvent>> read_input();

    void draw_cursor();
    void draw_status_message();

    void compute_cols_needed_for_line_numbers();

    String string_for_metadata(Edit::CharacterMetadata metadata) const;

    void print_char(char c, Edit::CharacterMetadata metadata);
    void flush_row(int line);

    Maybe<String> enter_prompt(const String& message, String starting_text = "");

    int index(int row, int col) const { return row * cols() + col; }

    Vector<Info> m_screen_info;
    Vector<bool> m_dirty_rows;
    String m_status_message;
    time_t m_status_message_time { 0 };
    String m_prompt_buffer;
    mutable String m_prev_clipboard_contents;
    mutable bool m_prev_clipboard_contents_were_whole_line { false };
    App::MousePressTracker m_mouse_press_tracker;
    Edit::CharacterMetadata m_last_metadata_rendered;
    int m_rows { 0 };
    int m_cols { 0 };
    int m_row_offset { 0 };
    int m_col_offset { 0 };
    int m_cols_needed_for_line_numbers { 0 };
    int m_exit_code { 0 };
    bool m_should_exit { false };
    bool m_show_status_bar { true };
};
