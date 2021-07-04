#pragma once

#include <edit/character_metadata.h>
#include <edit/forward.h>
#include <liim/string.h>
#include <liim/vector.h>

namespace Edit {
constexpr int tab_width = 4;

class Line {
public:
    explicit Line(String contents);
    ~Line();

    int length() const { return m_contents.size(); }
    bool empty() const { return m_contents.size() == 0; }

    const String& contents() const { return m_contents; }

    void insert_char_at(int position, char c);
    void remove_char_at(int position);

    void combine_line(Line& line);

    LineSplitResult split_at(int position);

    int col_position_of_index(const Document& document, const Panel& panel, int index) const;
    int index_of_col_position(const Document& document, const Panel& panel, int position) const;

    char char_at(int index) const { return contents()[index]; }

    int search(const String& text);

    void render(const Document& document, Panel& panel, int col_offset, int row_in_panel) const;

private:
    void compute_rendered_contents(const Document& document, const Panel& panel) const;

    String m_contents;
    mutable String m_rendered_contents;
    mutable Vector<int> m_rendered_sizes;
};

struct LineSplitResult {
    Line first;
    Line second;
};
}
