#pragma once

#include <liim/string.h>
#include <liim/vector.h>

class Panel;

struct KeyPress {
    enum Modifier {
        Shift = 1,
        Alt = 2,
        Control = 4,
    };

    enum Key {
        // Ascii keys are mapped to themselves
        LeftArrow = 1000,
        RightArrow,
        UpArrow,
        DownArrow,
        Home,
        End,

        Backspace = 2000,
        Delete,
        Enter,
        Insert,
        Escape,
        PageUp,
        PageDown,

        F0 = 3000,
        F1,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9,
        F10,
        F11,
        F12,
        F13,
        F14,
        F15,
        F16,
        F17,
        F18,
        F19,
        F20,
    };

    int modifiers;
    int key;
};

struct LineSplitResult;

class Line {
public:
    Line(String contents) : m_contents(move(contents)) {}

    int length() const { return m_contents.size(); }
    bool empty() const { return m_contents.size() == 0; }

    const String& contents() const { return m_contents; }

    void insert_char_at(int position, char c) { m_contents.insert(c, position); }
    void remove_char_at(int position) { m_contents.remove_index(position); }

    void combine_line(Line& line) { m_contents += line.contents(); }

    LineSplitResult split_at(int position);

    int col_position_of_index(int index) const;
    int index_of_col_position(int position) const;

private:
    String m_contents;
};

struct LineSplitResult {
    Line first;
    Line second;
};

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

    void render_line(int line, int row_in_panel) const;

    Line& line_at_cursor();
    const Line& line_at_cursor() const { return const_cast<Document&>(*this).line_at_cursor(); }
    int line_index_at_cursor() const;

    int cursor_col_position() const;

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
