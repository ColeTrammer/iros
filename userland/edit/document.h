#pragma once

#include <liim/pointers.h>
#include <liim/vector.h>

#include "line.h"
#include "selection.h"

class Command;
struct KeyPress;
class Panel;

enum class UpdateMaxCursorCol { No, Yes };

enum class MovementMode { Move, Select };

enum class DeleteCharMode { Backspace, Delete };

enum class LineMode { Single, Multiple };

class Document {
public:
    static UniquePtr<Document> create_from_file(const String& path, Panel& panel);
    static UniquePtr<Document> create_empty(Panel& panel);
    static UniquePtr<Document> create_single_line(Panel& panel);

    struct StateSnapshot {
        int row_offset { 0 };
        int col_offset { 0 };
        int cursor_row { 0 };
        int cursor_col { 0 };
        int max_cursor_col { 0 };
        bool document_was_modified { false };
        Selection selection;
    };

    struct Snapshot {
        Vector<Line> lines;
        StateSnapshot state;
    };

    Document(Vector<Line> lines, String name, Panel& panel, LineMode mode);
    ~Document();

    void display() const;

    Panel& panel() { return m_panel; }
    const Panel& panel() const { return m_panel; }

    void notify_key_pressed(KeyPress press);
    void notify_panel_size_changed();

    void save();
    void quit();

    bool single_line_mode() const { return m_line_mode == LineMode::Single; }

    String content_string() const;

    bool convert_tabs_to_spaces() const { return m_convert_tabs_to_spaces; }
    void set_convert_tabs_to_spaces(bool b) { m_convert_tabs_to_spaces = b; }

    bool needs_display() const { return m_needs_display; }
    void set_needs_display() { m_needs_display = true; }

    int cursor_col_position() const;
    int cursor_row_position() const;

    bool modified() const { return m_document_was_modified; }

    const String& name() const { return m_name; }

    const String& search_text() const { return m_search_text; }
    void set_search_text(String text);

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

    Line& line_at_cursor();
    const Line& line_at_cursor() const { return const_cast<Document&>(*this).line_at_cursor(); }
    int line_index_at_cursor() const;
    char char_at_cursor() const;
    int num_lines() const { return m_lines.size(); }

    bool cursor_at_document_start() const;
    bool cursor_at_document_end() const;

    void remove_line(int index);
    void insert_line(Line&& line, int index);

    void merge_lines(int l1, int l2);

    void set_was_modified(bool b) { m_document_was_modified = b; }

    Snapshot snapshot() const;
    void restore(Snapshot snapshot);

    StateSnapshot snapshot_state() const;
    void restore_state(const StateSnapshot& state_snapshot);

    const Selection& selection() const { return m_selection; }
    void delete_selection();
    void clear_selection();
    String selection_text() const;

    void move_cursor_to(int line_index, int index_into_line, MovementMode mode = MovementMode::Move);
    void insert_text_at_cursor(const String& string);

    bool show_line_numbers() const { return m_show_line_numbers; }
    void set_show_line_numbers(bool b);

    int row_offset() const { return m_row_offset; }
    int col_offset() const { return m_col_offset; }

private:
    int clamp_cursor_to_line_end();
    void update_selection_state_for_mode(MovementMode mode);

    void update_search_results();
    void clear_search_results();
    void enter_interactive_search();

    void split_line_at_cursor();
    void insert_char(char c);
    void delete_char(DeleteCharMode mode);
    void delete_word(DeleteCharMode mode);

    void redo();
    void undo();

    void copy();
    void paste();
    void cut();

    void go_to_line();

    void render_selection();
    void swap_selection_start_and_cursor();

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
        bool did_modify = command->execute();
        if (did_modify) {
            m_command_stack.add(move(command));
            m_command_stack_index++;
            m_document_was_modified = true;
        }
    }

    Vector<Line> m_lines;
    String m_name;
    Panel& m_panel;
    LineMode m_line_mode { LineMode::Multiple };

    Vector<UniquePtr<Command>> m_command_stack;
    int m_command_stack_index { 0 };

    String m_search_text;
    int m_search_result_count { 0 };

    int m_row_offset { 0 };
    int m_col_offset { 0 };
    int m_max_cursor_col { 0 };
    bool m_document_was_modified { false };
    Selection m_selection;

    int m_max_undo_stack { 50 };
    bool m_show_line_numbers { false };
    bool m_convert_tabs_to_spaces { true };
    mutable bool m_needs_display { false };
};
