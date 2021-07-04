#include <assert.h>
#include <edit/document.h>
#include <edit/line.h>
#include <edit/panel.h>

namespace Edit {
LineSplitResult Line::split_at(int position) {
    StringView first = StringView(contents().string(), contents().string() + position - 1);
    StringView second = StringView(contents().string() + position);

    auto l1 = Line(String(first));
    auto l2 = Line(String(second));

    return { move(l1), move(l2) };
}

Line::Line(String contents) : m_contents(move(contents)) {}

Line::~Line() {}

int Line::col_position_of_index(int index) const {
    int col = 0;
    for (int i = 0; i < index; i++) {
        char c = contents()[i];
        if (c == '\t') {
            col += tab_width - (col % tab_width);
        } else {
            col++;
        }
    }
    return col;
}

int Line::index_of_col_position(int position) const {
    int col = 0;
    int index;
    for (index = 0; index < length(); index++) {
        char c = contents()[index];
        int col_width = 1;
        if (c == '\t') {
            col_width = tab_width - (col % tab_width);
        }

        col += col_width;
        if (col > position) {
            break;
        }
    }
    return index;
}

void Line::insert_char_at(int position, char c) {
    m_contents.insert(c, position);
}

void Line::remove_char_at(int position) {
    m_contents.remove_index(position);
}

void Line::combine_line(Line& line) {
    m_contents += line.contents();
}

int Line::search(const String& text) {
    char* s = m_contents.string();
    int matches = 0;
    for (;;) {
        char* match = strstr(s, text.string());
        if (!match) {
            break;
        }

        s = match + text.size();
        matches++;
    }

    return matches;
}

void Line::compute_rendered_contents(Document& document, Panel& panel) const {
    m_rendered_contents.clear();

    int col_position = 0;
    for (int line_index = 0; line_index < length(); line_index++) {
        char c = char_at(line_index);
        if (c == '\t') {
            int num_spaces = tab_width - (col_position % tab_width);
            for (int i = 0; i < num_spaces; i++) {
                m_rendered_contents += String(' ');
            }
            col_position += num_spaces;
        } else {
            m_rendered_contents += String(c);
            col_position++;
        }

        if (document.preview_auto_complete() && document.selection().empty() && this == &document.line_at_cursor() &&
            line_index + 1 == document.index_into_line_at_cursor()) {
            auto suggestions = panel.get_suggestions();
            if (suggestions.suggestion_count() == 1) {
                auto& text = suggestions.suggestion_list().first();
                int length_to_write = text.size() - suggestions.suggestion_offset();
                for (int i = 0; i < length_to_write; i++) {
                    m_rendered_contents += String(text[suggestions.suggestion_offset() + i]);
                }
                col_position += length_to_write;
            }
        }
    }
}

void Line::render(Document& document, Panel& panel, int col_offset, int row_in_panel) const {
    compute_rendered_contents(document, panel);

    int col_position;
    for (col_position = 0;
         static_cast<size_t>(col_offset + col_position) < m_rendered_contents.size() && col_position < panel.cols_at_row(row_in_panel);
         col_position++) {
        panel.set_text_at(row_in_panel, col_position, m_rendered_contents[col_offset + col_position], {});
    }

    for (; col_position < panel.cols_at_row(row_in_panel); col_position++) {
        panel.set_text_at(row_in_panel, col_position, ' ', CharacterMetadata());
    }
}
}
