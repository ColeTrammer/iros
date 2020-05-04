#pragma once

#include <liim/string.h>
#include <liim/vector.h>

class Panel;

struct KeyPress {
    enum Modifier {
        Control,
        Alt,
    };

    enum Key {
        // Ascii keys are mapped to themselves
        LeftArrow = 1000,
        RightArrow = 1001,
        UpArrow = 1002,
        DownArrow = 1003,
    };

    int modifiers;
    int key;
};

class Line {
public:
    Line(String contents) : m_contents(move(contents)) {}

    const String& contents() const { return m_contents; }

private:
    String m_contents;
};

class Document {
public:
    static UniquePtr<Document> create_from_file(const String& path, Panel& panel);

    Document(Vector<Line> lines, String name, Panel& panel) : m_lines(move(lines)), m_name(move(name)), m_panel(panel) {}

    void display() const;

    Panel& panel() { return m_panel; }
    const Panel& panel() const { return m_panel; }

    void notify_key_pressed(KeyPress press);

private:
    void move_cursor_left();
    void move_cursor_right();
    void move_cursor_down();
    void move_cursor_up();

    void render_line(int line, int row_in_panel) const;

    Vector<Line> m_lines;
    String m_name;
    Panel& m_panel;
    int m_row_offset { 0 };
    int m_col_offset { 0 };
};
