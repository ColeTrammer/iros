#pragma once

#include <liim/pointers.h>
#include <liim/vector.h>

#include "line.h"

struct KeyPress;
class Panel;

enum class UpdateMaxCursorCol { No, Yes };

enum class DeleteCharMode { Backspace, Delete };

enum class LineMode { Single, Multiple };

class Document {
public:
    static UniquePtr<Document> create_from_file(const String& path, Panel& panel);
    static UniquePtr<Document> create_empty(Panel& panel);
    static UniquePtr<Document> create_single_line(Panel& panel);

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

private:
    void move_cursor_left();
    void move_cursor_right();
    void move_cursor_down();
    void move_cursor_up();
    void move_cursor_to_line_start();
    void move_cursor_to_line_end(UpdateMaxCursorCol update = UpdateMaxCursorCol::Yes);
    void clamp_cursor_to_line_end();
    void split_line_at_cursor();

    void insert_char(char c);
    void delete_char(DeleteCharMode mode);

    void merge_lines(int l1, int l2);

    Line& line_at_cursor();
    const Line& line_at_cursor() const { return const_cast<Document&>(*this).line_at_cursor(); }
    int line_index_at_cursor() const;

    Vector<Line> m_lines;
    String m_name;
    Panel& m_panel;
    LineMode m_line_mode { LineMode::Multiple };
    int m_row_offset { 0 };
    int m_col_offset { 0 };
    int m_max_cursor_col { 0 };
    bool m_convert_tabs_to_spaces { true };
    mutable bool m_needs_display { false };
    bool m_document_was_modified { false };
};
