#pragma once

#include <edit/forward.h>
#include <liim/string.h>

namespace Edit {
constexpr int tab_width = 4;

class Line {
public:
    explicit Line(String contents);
    ~Line();

    enum class OverwriteFrom { None, LineStart, LineEnd };
    void overwrite(Document& document, Line&& line, int this_line_index, OverwriteFrom mode);

    int length() const { return m_contents.size(); }
    bool empty() const { return m_contents.size() == 0; }

    const String& contents() const { return m_contents; }

    void insert_char_at(Document& document, const TextIndex& index, char c);
    void remove_char_at(Document& document, const TextIndex& index);

    void combine_line(Document& document, Line& line);

    LineSplitResult split_at(int position);

    RelativePosition relative_position_of_index(const Document& document, Display& display, const TextIndex& index) const;
    int absoulte_col_offset_of_index(const Document& document, Display& display, const TextIndex& index) const;
    TextIndex index_of_relative_position(const Document& document, Display& display, int this_line_index,
                                         const RelativePosition& position) const;
    int max_col_in_relative_row(const Document& document, Display& display, int this_line_index, int relative_row) const;
    int rendered_line_count(const Document& document, Display& display, int this_line_index) const;

    int next_index_into_line(const Document& document, Display& display, const TextIndex& index) const;
    int prev_index_into_line(const Document& document, Display& display, const TextIndex& index) const;

    char char_at(int index) const { return contents()[index]; }

    void search(const Document& document, int this_line_index, const String& text, TextRangeCollection& results) const;

    int render(const Document& document, Display& display, int this_line_index, int col_offset, int relative_row_start,
               int row_in_display) const;

private:
    const RenderedLine& compute_rendered_contents(const Document& document, Display& display, int this_line_index) const;

    enum class RangeFor {
        Text,
        Cursor,
    };

    const PositionRange* range_for_relative_position(const Document& document, Display& display, int this_line_index,
                                                     const RelativePosition& position) const;
    const PositionRange* range_for_index(const Document& documnet, Display& display, const TextIndex& index, RangeFor mode) const;

    String m_contents;
};

struct LineSplitResult {
    Line first;
    Line second;
};
}
