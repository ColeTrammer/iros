#pragma once

#include <edit/document_type.h>
#include <edit/forward.h>
#include <edit/line.h>
#include <edit/multicursor.h>
#include <edit/suggestions.h>
#include <edit/text_index.h>
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
    static SharedPtr<Document> create_from_stdin(const String& path, Display& display);
    static SharedPtr<Document> create_from_file(const String& path, Display& display);
    static SharedPtr<Document> create_from_text(const String& text);
    static SharedPtr<Document> create_empty();
    static SharedPtr<Document> create_single_line(String text = "");

    struct StateSnapshot {
        MultiCursor cursors;
        bool document_was_modified { false };
    };

    struct Snapshot {
        Vector<Line> lines;
        StateSnapshot state;
    };

    Document(Vector<Line> lines, String name, InputMode mode);
    ~Document();

    void copy_settings_from(const Document& other);

    void display(Display& display) const;
    void set_needs_display();

    void invalidate_rendered_contents(const Line& line);
    void invalidate_all_rendered_contents();

    void notify_key_pressed(Display& display, const App::KeyEvent& event);
    bool notify_mouse_event(Display& display, const App::MouseEvent& event);
    void notify_display_size_changed();

    void save(Display& display);
    void quit(Display& display);

    bool input_text_mode() const { return m_input_mode == InputMode::InputText; }
    bool submittable() const { return m_submittable; }

    void set_submittable(bool b) { m_submittable = b; }

    void set_auto_complete_mode(AutoCompleteMode mode) { m_auto_complete_mode = mode; }

    bool preview_auto_complete() const { return m_preview_auto_complete; }
    void set_preview_auto_complete(bool b);

    String content_string() const;
    size_t cursor_index_in_content_string(const Cursor& cursor) const;

    bool convert_tabs_to_spaces() const { return m_convert_tabs_to_spaces; }
    void set_convert_tabs_to_spaces(bool b) { m_convert_tabs_to_spaces = b; }

    bool word_wrap_enabled() const { return m_word_wrap_enabled; }
    void set_word_wrap_enabled(bool b);

    bool modified() const { return m_document_was_modified; }

    const String& name() const { return m_name; }
    void set_name(String name) { m_name = move(name); }

    const String& search_text() const { return m_search_text; }
    void set_search_text(String text);
    int search_result_count() const { return m_search_results.size(); }
    void move_cursor_to_next_search_match(Display& display, Cursor& cursor);

    void move_cursor_left(Display& display, Cursor& cursor, MovementMode mode = MovementMode::Move);
    void move_cursor_right(Display& display, Cursor& cursor, MovementMode mode = MovementMode::Move);
    void move_cursor_down(Display& display, Cursor& cursor, MovementMode mode = MovementMode::Move);
    void move_cursor_up(Display& display, Cursor& cursor, MovementMode mode = MovementMode::Move);
    void move_cursor_to_line_start(Display& display, Cursor& cursor, MovementMode mode = MovementMode::Move);
    void move_cursor_to_line_end(Display& display, Cursor& cursor, MovementMode mode = MovementMode::Move);
    void move_cursor_left_by_word(Display& display, Cursor& cursor, MovementMode mode = MovementMode::Move);
    void move_cursor_right_by_word(Display& display, Cursor& cursor, MovementMode mode = MovementMode::Move);
    void move_cursor_to_document_start(Display& display, Cursor& cursor, MovementMode mode = MovementMode::Move);
    void move_cursor_to_document_end(Display& display, Cursor& cursor, MovementMode mode = MovementMode::Move);
    void move_cursor_page_up(Display& display, Cursor& cursor, MovementMode mode = MovementMode::Move);
    void move_cursor_page_down(Display& display, Cursor& cursor, MovementMode mode = MovementMode::Move);
    void move_cursor_to(Display& display, Cursor& cursor, const TextIndex& index, MovementMode mode = MovementMode::Move);
    void clamp_cursor_to_line_end(Display& display, Cursor& cursor);

    void scroll_cursor_into_view(Display& display, Cursor& cursor);

    TextIndex text_index_at_absolute_position(Display& display, const Position& position) const;
    TextIndex text_index_at_scrolled_position(Display& display, const Position& position) const;
    Position relative_to_absolute_position(Display& display, const Line& line, const Position& line_relative_position) const;
    int index_of_line(const Line& line) const;
    int num_lines() const { return m_lines.size(); }
    int num_rendered_lines(Display& display) const;

    Position cursor_position_on_display(Display& display, Cursor& cursor) const;

    void remove_line(int index);
    void insert_line(Line&& line, int index);
    void rotate_lines_up(int start, int end);
    void rotate_lines_down(int start, int end);

    void split_line_at(const TextIndex& index);
    void merge_lines(int first_line_index, int second_line_index);

    void set_was_modified(bool b) { m_document_was_modified = b; }

    void finish_input(Display& display, bool should_scroll_cursor_into_view);

    Snapshot snapshot(Display& display) const;
    void restore(MultiCursor& cursors, Snapshot snapshot);

    StateSnapshot snapshot_state(Display& display) const;
    void restore_state(MultiCursor& cursors, const StateSnapshot& state_snapshot);

    void delete_selection(Cursor& cursor);
    void clear_selection(Cursor& cursor);
    String selection_text(Cursor& cursor) const;

    void select_next_word_at_cursor(Display& display);
    void select_line_at_cursor(Display& display, Cursor& cursor);
    void select_word_at_cursor(Display& display, Cursor& cursor);
    void select_all(Display& display, Cursor& cursor);

    void redo(Display& display);
    void undo(Display& display);

    void copy(Display& display, MultiCursor& cursor);
    void paste(Display& display, MultiCursor& cursor);
    void cut(Display& display, MultiCursor& cursor);

    void insert_text_at_cursor(Display& display, const String& string);

    bool show_line_numbers() const { return m_show_line_numbers; }
    void set_show_line_numbers(bool b);

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

    void did_delete_lines(int line_index, int line_count);
    void did_add_lines(int line_index, int line_count);
    void did_split_line(int line_index, int index_into_line);
    void did_merge_lines(int first_line_index, int first_line_length, int second_line_index);
    void did_add_to_line(int line_index, int index_into_line, int bytes_added);
    void did_delete_from_line(int line_index, int index_into_line, int bytes_deleted);

    bool execute_command(Display& display, Command& command);

    TextRangeCollection& syntax_highlighting_info() { return m_syntax_highlighting_info; }
    const TextRangeCollection& syntax_highlighting_info() const { return m_syntax_highlighting_info; }

    void register_display(Display& display);
    void unregister_display(Display& display);

    Function<void()> on_change;
    Function<void()> on_submit;
    Function<void()> on_escape_press;

private:
    void update_selection_state_for_mode(Cursor& cursor, MovementMode mode);

    void update_search_results();
    void clear_search_results();
    void enter_interactive_search(Display& display);

    void update_syntax_highlighting();

    void swap_lines_at_cursor(Display& display, SwapDirection direction);
    void split_line_at_cursor(Display& display);
    void insert_char(Display& display, char c);
    void delete_char(Display& display, DeleteCharMode mode);
    void delete_word(Display& display, DeleteCharMode mode);

    void go_to_line(Display& display);

    void swap_selection_start_and_cursor(Display& display, Cursor& cursor);

    void guess_type_from_name();

    template<typename C, typename... Args>
    void push_command(Display& display, Args... args) {
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

        auto command = make_unique<C>(*this, display, forward<Args>(args)...);
        bool did_modify = execute_command(display, *command);
        if (did_modify) {
            m_command_stack.add(move(command));
            m_command_stack_index++;
            m_document_was_modified = true;
            update_search_results();
            update_syntax_highlighting();
            set_needs_display();

            if (on_change) {
                on_change();
            }
        }
    }

    Vector<Line> m_lines;
    String m_name;
    DocumentType m_type { DocumentType::Text };
    InputMode m_input_mode { InputMode::Document };
    bool m_submittable { false };

    AutoCompleteMode m_auto_complete_mode { AutoCompleteMode::Never };
    bool m_preview_auto_complete { false };

    Vector<UniquePtr<Command>> m_command_stack;
    int m_command_stack_index { 0 };
    int m_max_undo_stack { 50 };
    bool m_document_was_modified { false };

    String m_search_text;
    TextRangeCollection m_search_results;
    int m_search_result_index { 0 };

    TextRangeCollection m_syntax_highlighting_info;

    Vector<Display*> m_displays;

    bool m_word_wrap_enabled { true };
    bool m_show_line_numbers { false };
    bool m_convert_tabs_to_spaces { true };
};
}
