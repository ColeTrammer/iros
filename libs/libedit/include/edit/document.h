#pragma once

#include <edit/cursor.h>
#include <edit/document_type.h>
#include <edit/forward.h>
#include <edit/line.h>
#include <edit/selection.h>
#include <edit/suggestions.h>
#include <edit/text_range_collection.h>
#include <eventloop/forward.h>
#include <liim/function.h>
#include <liim/pointers.h>
#include <liim/vector.h>

namespace Edit {
enum class UpdateMaxCursorCol { No, Yes };

enum class MovementMode { Move, Select };

enum class DeleteCharMode { Backspace, Delete };

enum class SwapDirection { Up, Down };

enum class InputMode { Document, InputText };

enum class AutoCompleteMode { Never, Always };

class Document {
public:
    static UniquePtr<Document> create_from_stdin(const String& path, Panel& panel);
    static UniquePtr<Document> create_from_file(const String& path, Panel& panel);
    static UniquePtr<Document> create_from_text(Panel& panel, const String& text);
    static UniquePtr<Document> create_empty(Panel& panel);
    static UniquePtr<Document> create_single_line(Panel& panel, String text = "");

    struct StateSnapshot {
        Cursor cursor;
        int max_cursor_col { 0 };
        bool document_was_modified { false };
        Selection selection;
    };

    struct Snapshot {
        Vector<Line> lines;
        StateSnapshot state;
    };

    Document(Vector<Line> lines, String name, Panel& panel, InputMode mode);
    ~Document();

    void copy_settings_from(const Document& other);

    void display() const;
    void display_if_needed() const;

    Panel& panel() { return m_panel; }
    const Panel& panel() const { return m_panel; }

    void notify_key_pressed(KeyPress press);
    bool notify_mouse_event(const App::MouseEvent& event);
    void notify_panel_size_changed();

    void save();
    void quit();

    bool input_text_mode() const { return m_input_mode == InputMode::InputText; }
    bool submittable() const { return m_submittable; }

    void set_submittable(bool b) { m_submittable = b; }

    void set_auto_complete_mode(AutoCompleteMode mode) { m_auto_complete_mode = mode; }

    bool preview_auto_complete() const { return m_preview_auto_complete; }
    void set_preview_auto_complete(bool b) { m_preview_auto_complete = b; }

    String content_string() const;
    size_t cursor_index_in_content_string() const;

    bool convert_tabs_to_spaces() const { return m_convert_tabs_to_spaces; }
    void set_convert_tabs_to_spaces(bool b) { m_convert_tabs_to_spaces = b; }

    bool needs_display() const { return m_needs_display; }
    void set_needs_display() { m_needs_display = true; }

    bool modified() const { return m_document_was_modified; }

    const String& name() const { return m_name; }
    void set_name(String name) { m_name = move(name); }

    const String& search_text() const { return m_search_text; }
    void set_search_text(String text);
    int search_result_count() const { return m_search_result_count; }
    void move_cursor_to_next_search_match();

    void move_cursor_left(MovementMode mode = MovementMode::Move);
    void move_cursor_right(MovementMode mode = MovementMode::Move);
    void move_cursor_down(MovementMode mode = MovementMode::Move);
    void move_cursor_up(MovementMode mode = MovementMode::Move);
    void move_cursor_to_line_start(MovementMode mode = MovementMode::Move);
    void move_cursor_to_line_end(MovementMode mode = MovementMode::Move);
    void move_cursor_left_by_word(MovementMode mode = MovementMode::Move);
    void move_cursor_right_by_word(MovementMode mode = MovementMode::Move);
    void move_cursor_to_document_start(MovementMode mode = MovementMode::Move);
    void move_cursor_to_document_end(MovementMode mode = MovementMode::Move);
    void move_cursor_page_up(MovementMode mode = MovementMode::Move);
    void move_cursor_page_down(MovementMode mode = MovementMode::Move);

    void scroll(int vertical, int horizontal);
    void scroll_up(int times = 1);
    void scroll_down(int times = 1);
    void scroll_left(int times = 1);
    void scroll_right(int times = 1);
    void scroll_cursor_into_view();

    Line& line_at_cursor();
    const Line& line_at_cursor() const { return const_cast<Document&>(*this).line_at_cursor(); }
    int index_into_line_at_cursor() const;
    int index_into_line(int index_of_line, int position) const;
    int index_of_line_at_cursor() const;
    int index_of_line_at_position(int position) const;
    char char_at_cursor() const;
    int num_lines() const { return m_lines.size(); }

    int cursor_col_on_panel() const;
    int cursor_row_on_panel() const;

    bool cursor_at_document_start() const;
    bool cursor_at_document_end() const;

    void remove_line(int index);
    void insert_line(Line&& line, int index);
    void rotate_lines_up(int start, int end);
    void rotate_lines_down(int start, int end);

    void merge_lines(int l1, int l2);

    void set_was_modified(bool b) { m_document_was_modified = b; }

    void finish_key_press();

    Snapshot snapshot() const;
    void restore(Snapshot snapshot);

    StateSnapshot snapshot_state() const;
    void restore_state(const StateSnapshot& state_snapshot);

    const Selection& selection() const { return m_selection; }
    void set_selection(Selection selection) { m_selection = move(selection); }
    void delete_selection();
    void clear_selection();
    String selection_text() const;

    void select_line_at_cursor();
    void select_word_at_cursor();
    void select_all();

    void redo();
    void undo();

    void copy();
    void paste();
    void cut();

    void move_cursor_to(int line_index, int index_into_line, MovementMode mode = MovementMode::Move);
    void insert_text_at_cursor(const String& string);

    bool show_line_numbers() const { return m_show_line_numbers; }
    void set_show_line_numbers(bool b);

    int row_offset() const { return m_row_offset; }
    int col_offset() const { return m_col_offset; }

    void clear_search();

    void set_input_mode(InputMode mode) { m_input_mode = mode; }

    DocumentType type() const { return m_type; }
    void set_type(DocumentType type);

    Line& line_at_index(int index) { return m_lines[index]; }
    const Line& line_at_index(int index) const { return m_lines[index]; }

    Line& first_line() { return m_lines.first(); }
    const Line& first_line() const { return m_lines.first(); }

    Line& last_line() { return m_lines.last(); }
    const Line& last_line() const { return m_lines.last(); }

    bool execute_command(Command& command);

    TextRangeCollection& syntax_highlighting_info() { return m_syntax_highlighting_info; }
    const TextRangeCollection& syntax_highlighting_info() const { return m_syntax_highlighting_info; }

    Function<void()> on_change;
    Function<void()> on_submit;
    Function<void()> on_escape_press;

private:
    int clamp_cursor_to_line_end();
    void update_selection_state_for_mode(MovementMode mode);

    void update_search_results();
    void clear_search_results();
    void enter_interactive_search();

    void update_syntax_highlighting();

    void swap_lines_at_cursor(SwapDirection direction);
    void split_line_at_cursor();
    void insert_char(char c);
    void delete_char(DeleteCharMode mode);
    void delete_word(DeleteCharMode mode);

    void go_to_line();

    void swap_selection_start_and_cursor();

    void guess_type_from_name();

    template<typename C, typename... Args>
    void push_command(Args... args) {
        // This means some undo's have taken place, and the user started typing
        // something else, so the redo stack will be discarded.
        if (m_command_stack_index != m_command_stack.size()) {
            m_command_stack.resize(m_command_stack_index);
        }

        if (m_command_stack.size() >= m_max_undo_stack) {
            // FIXME: this makes the Vector data structure very inefficent
            //        a doubly-linked list would be much nicer.
            m_command_stack.remove(0);
            m_command_stack_index--;
        }

        auto command = make_unique<C>(*this, forward<Args>(args)...);
        bool did_modify = execute_command(*command);
        if (did_modify) {
            m_command_stack.add(move(command));
            m_command_stack_index++;
            m_document_was_modified = true;
            update_search_results();
            update_syntax_highlighting();

            if (on_change) {
                on_change();
            }
        }
    }

    Vector<Line> m_lines;
    String m_name;
    Panel& m_panel;
    InputMode m_input_mode { InputMode::Document };
    bool m_submittable { false };

    AutoCompleteMode m_auto_complete_mode { AutoCompleteMode::Never };
    bool m_preview_auto_complete { false };

    Vector<UniquePtr<Command>> m_command_stack;
    int m_command_stack_index { 0 };

    String m_search_text;
    int m_search_result_count { 0 };

    TextRangeCollection m_syntax_highlighting_info;

    int m_row_offset { 0 };
    int m_col_offset { 0 };
    int m_max_cursor_col { 0 };
    bool m_document_was_modified { false };
    Selection m_selection;
    Cursor m_cursor;

    int m_max_undo_stack { 50 };
    bool m_show_line_numbers { false };
    bool m_convert_tabs_to_spaces { true };
    mutable bool m_needs_display { false };
    DocumentType m_type { DocumentType::Text };
};
}
