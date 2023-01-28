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

    void insert_text(Document& document, const TextIndex& index, StringView text);
    void remove_count(Document& document, const TextIndex& index, int count);

    void combine_line(Document& document, Line& line);

    LineSplitResult split_at(int position);

    char char_at(int index) const { return contents()[index]; }

    void search(const Document& document, int this_line_index, const String& text, TextRangeCollection& results) const;

private:
    String m_contents;
};

struct LineSplitResult {
    Line first;
    Line second;
};
}
