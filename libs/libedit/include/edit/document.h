#pragma once

#include <edit/document_type.h>
#include <edit/forward.h>
#include <edit/line.h>
#include <edit/multicursor.h>
#include <edit/suggestions.h>
#include <edit/text_index.h>
#include <edit/text_range_collection.h>
#include <eventloop/event.h>
#include <eventloop/object.h>
#include <liim/function.h>
#include <liim/pointers.h>
#include <liim/vector.h>

APP_EVENT(Edit, DeleteLines, App::Event, (), ((int, line_index), (int, line_count)), ())
APP_EVENT(Edit, AddLines, App::Event, (), ((int, line_index), (int, line_count)), ())
APP_EVENT(Edit, SplitLines, App::Event, (), ((int, line_index), (int, index_into_line)), ())
APP_EVENT(Edit, MergeLines, App::Event, (), ((int, first_line_index), (int, first_line_length), (int, second_line_index)), ())
APP_EVENT(Edit, AddToLine, App::Event, (), ((int, line_index), (int, index_into_line), (int, bytes_added)), ())
APP_EVENT(Edit, DeleteFromLine, App::Event, (), ((int, line_index), (int, index_into_line), (int, bytes_deleted)), ())
APP_EVENT(Edit, MoveLineTo, App::Event, (), ((int, line), (int, destination)), ())

APP_EVENT(Edit, SyntaxHighlightingChanged, App::Event, (), (), ())

APP_EVENT(Edit, Submit, App::Event, (), (), ())
APP_EVENT(Edit, Change, App::Event, (), (), ())

namespace Edit {
class DeleteCommand;

enum class MovementMode { Move, Select };

enum class DeleteCharMode { Backspace, Delete };

enum class SwapDirection { Up, Down };

enum class InputMode { Document, InputText };

class Document final : public App::Object {
    APP_OBJECT(Document)

    APP_EMITS(App::Object, DeleteLines, AddLines, SplitLines, MergeLines, AddToLine, DeleteFromLine, SyntaxHighlightingChanged, MoveLineTo,
              Submit, Change)

public:
    static SharedPtr<Document> create_from_stdin(const String& path, Maybe<String>& error_message);
    static SharedPtr<Document> create_from_file(const String& path, Maybe<String>& error_message);
    static SharedPtr<Document> create_from_text(const String& text);
    static SharedPtr<Document> create_empty();

    struct StateSnapshot {
        MultiCursor::Snapshot cursors;
        bool document_was_modified { false };
    };

    struct Snapshot {
        Vector<Line> lines;
        StateSnapshot state;
    };

    virtual ~Document() override;

    void copy_settings_from(const Document& other);

    void display(Display& display) const;

    void invalidate_lines_in_range(Display& display, const TextRange& range);
    void invalidate_lines_in_range_collection(Display& display, const TextRangeCollection& collection);

    App::ObjectBoundCoroutine save(Display& display);
    App::ObjectBoundCoroutine quit(Display& display);

    bool input_text_mode() const { return m_input_mode == InputMode::InputText; }
    bool submittable() const { return m_submittable; }

    void set_submittable(bool b) { m_submittable = b; }

    String content_string() const;
    size_t cursor_index_in_content_string(const Cursor& cursor) const;

    bool convert_tabs_to_spaces() const { return m_convert_tabs_to_spaces; }
    void set_convert_tabs_to_spaces(bool b) { m_convert_tabs_to_spaces = b; }

    bool modified() const { return m_document_was_modified; }

    const String& name() const { return m_name; }
    void set_name(String name) { m_name = move(name); }

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
    void move_line_to(int line, int destination);

    void split_line_at(const TextIndex& index);
    void merge_lines(int first_line_index, int second_line_index);

    void set_was_modified(bool b) { m_document_was_modified = b; }

    void start_input(Display& display, bool should_save_cursor_state);
    void finish_input(Display& display, bool should_scroll_cursor_into_view);

    Snapshot snapshot(Display& display) const;
    void restore(MultiCursor& cursors, Snapshot snapshot);

    StateSnapshot snapshot_state(Display& display) const;
    void restore_state(MultiCursor& cursors, const StateSnapshot& state_snapshot);

    void delete_selection(Cursor& cursor);
    void clear_selection(Cursor& cursor);
    String selection_text(const Cursor& cursor) const;

    String text_in_range(const TextIndex& start, const TextIndex& end) const;

    void select_line_at_cursor(Display& display, Cursor& cursor);
    void select_word_at_cursor(Display& display, Cursor& cursor);
    void select_all(Display& display, Cursor& cursor);

    void redo(Display& display);
    void undo(Display& display);

    void copy(Display& display, MultiCursor& cursor);
    void paste(Display& display, MultiCursor& cursor);
    void cut(Display& display, MultiCursor& cursor);

    void insert_suggestion(Display& display, const MatchedSuggestion& suggestion);
    void insert_text_at_cursor(Display& display, const String& string);

    void set_input_mode(InputMode mode) { m_input_mode = mode; }

    DocumentType type() const { return m_type; }
    void set_type(DocumentType type);

    Line& line_at_index(int index) { return m_lines[index]; }
    const Line& line_at_index(int index) const { return m_lines[index]; }

    Line& first_line() { return m_lines.first(); }
    const Line& first_line() const { return m_lines.first(); }

    Line& last_line() { return m_lines.last(); }
    const Line& last_line() const { return m_lines.last(); }

    bool execute_command(Display& display, Command& command);

    TextRangeCollection& syntax_highlighting_info() { return m_syntax_highlighting_info; }
    const TextRangeCollection& syntax_highlighting_info() const { return m_syntax_highlighting_info; }

    void update_syntax_highlighting();

    void register_display(Display& display);
    void unregister_display(Display& display, bool remove_listener);

    void swap_lines_at_cursor(Display& display, SwapDirection direction);
    void split_line_at_cursor(Display& display);
    void insert_char(Display& display, char c);
    void delete_char(Display& display, DeleteCharMode mode);
    void delete_word(Display& display, DeleteCharMode mode);

    App::ObjectBoundCoroutine go_to_line(Display& display);

private:
    Document(Vector<Line> lines, String name, InputMode mode);

    void update_selection_state_for_mode(Cursor& cursor, MovementMode mode);

    void update_suggestions(Display& display);

    void swap_selection_start_and_cursor(Display& display, Cursor& cursor);

    void guess_type_from_name();

    template<typename C, typename... Args>
    void push_command(Display& display, Args... args) {
        auto command = make_unique<C>(*this, forward<Args>(args)...);
        push_command(display, move(command));
    }

    void push_command(Display& display, UniquePtr<Command> command);

    Vector<Line> m_lines;
    String m_name;
    DocumentType m_type { DocumentType::Text };
    InputMode m_input_mode { InputMode::Document };
    bool m_submittable { false };
    bool m_convert_tabs_to_spaces { true };

    Vector<UniquePtr<Command>> m_command_stack;
    int m_command_stack_index { 0 };
    int m_max_undo_stack { 50 };
    bool m_document_was_modified { false };

    TextRangeCollection m_syntax_highlighting_info;

    Vector<Display*> m_displays;
};
}
