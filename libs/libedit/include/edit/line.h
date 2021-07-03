#pragma once

#include <edit/character_metadata.h>
#include <edit/forward.h>
#include <liim/string.h>
#include <liim/vector.h>

namespace Edit {
constexpr int tab_width = 4;

class Line {
public:
    Line(String contents);
    ~Line();

    int length() const { return m_contents.size(); }
    bool empty() const { return m_contents.size() == 0; }

    const String& contents() const { return m_contents; }
    const Vector<CharacterMetadata>& metadata() const { return m_metadata; }

    void insert_char_at(int position, char c);
    void remove_char_at(int position);

    void combine_line(Line& line);

    LineSplitResult split_at(int position);

    int col_position_of_index(int index) const;
    int index_of_col_position(int position) const;

    char char_at(int index) const { return contents()[index]; }

    CharacterMetadata& metadata_at(int index) { return m_metadata[index]; }
    const CharacterMetadata& metadata_at(int index) const { return m_metadata[index]; }

    int search(const String& text);
    void clear_search();

    void clear_selection();
    void toggle_select_after(int index);
    void toggle_select_before(int index);
    void select_all();

    void clear_syntax_highlighting();

    void render(Document& document, Panel& panel, int col_offset, int row_in_panel) const;

private:
    void compute_rendered_contents(Document& document, Panel& panel) const;

    String m_contents;
    mutable String m_rendered_contents;
    Vector<CharacterMetadata> m_metadata;
    mutable Vector<CharacterMetadata> m_rendered_metadata;
};

struct LineSplitResult {
    Line first;
    Line second;
};
}
