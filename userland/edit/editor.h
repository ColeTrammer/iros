#pragma once

#include <liim/string.h>
#include <liim/vector.h>

class Panel;

struct KeyPress {
    enum Modifier {
        Control = 1,
        Alt = 2,
    };

    enum Key {
        // Ascii keys are mapped to themselves
        LeftArrow = 1000,
        RightArrow = 1001,
        UpArrow = 1002,
        DownArrow = 1003,
        Home = 1004,
        End = 1005,

        Backspace = 2000,
        Delete = 2001,
        Enter = 2002,
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

private:
    String m_contents;
};

struct LineSplitResult {
    Line first;
    Line second;
};

enum class UpdateMaxCursorCol { No, Yes };

enum class DeleteCharMode { Backspace, Delete };

class Document {
public:
    static UniquePtr<Document> create_from_file(const String& path, Panel& panel);

    Document(Vector<Line> lines, String name, Panel& panel) : m_lines(move(lines)), m_name(move(name)), m_panel(panel) {}

    void display() const;

    Panel& panel() { return m_panel; }
    const Panel& panel() const { return m_panel; }

    void notify_key_pressed(KeyPress press);

    void save();

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

    Vector<Line> m_lines;
    String m_name;
    Panel& m_panel;
    int m_row_offset { 0 };
    int m_col_offset { 0 };
    int m_max_cursor_col { 0 };
};
