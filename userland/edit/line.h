#pragma once

#include <liim/string.h>

constexpr int tab_width = 4;

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