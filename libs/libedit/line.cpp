#include <assert.h>
#include <edit/character_metadata.h>
#include <edit/display.h>
#include <edit/document.h>
#include <edit/line.h>
#include <edit/rendered_line.h>

namespace Edit {
LineSplitResult Line::split_at(int position) {
    auto first = contents().view().first(position);
    auto second = contents().view().substring(position);

    auto l1 = Line(String(first));
    auto l2 = Line(String(second));

    return { move(l1), move(l2) };
}

Line::Line(String contents) : m_contents(move(contents)) {}

Line::~Line() {}

void Line::overwrite(Document& document, Line&& line, int this_line_index, OverwriteFrom mode) {
    auto old_length = this->length();
    auto delta_length = line.length() - old_length;
    this->m_contents = move(line.contents());
    if (mode == OverwriteFrom::None) {
        return;
    }

    auto start_index = mode == OverwriteFrom::LineStart ? 0 : min(old_length, length());

    // Maybe a separate overwrite event is better since these events don't tell the whole picture.
    if (delta_length < 0) {
        document.emit<DeleteFromLine>(this_line_index, start_index, -delta_length);
    } else if (delta_length >= 0) {
        document.emit<AddToLine>(this_line_index, start_index, delta_length);
    }
}

void Line::insert_char_at(Document& document, const TextIndex& index, char c) {
    m_contents.insert(c, index.index_into_line());
    document.emit<AddToLine>(index.line_index(), index.index_into_line(), 1);
}

void Line::remove_char_at(Document& document, const TextIndex& index) {
    m_contents.remove_index(index.index_into_line());
    document.emit<DeleteFromLine>(index.line_index(), index.index_into_line(), 1);
}

void Line::combine_line(Document&, Line& line) {
    m_contents += line.contents();
}

void Line::search(const Document&, int this_line_index, const String& text, TextRangeCollection& results) const {
    int index_into_line = 0;
    for (;;) {
        const char* match = strstr(m_contents.string() + index_into_line, text.string());
        if (!match) {
            break;
        }

        index_into_line = match - m_contents.string();
        results.add({ { this_line_index, index_into_line },
                      { this_line_index, index_into_line + static_cast<int>(text.size()) },
                      { CharacterMetadata::Flags::Highlighted } });
        index_into_line += text.size();
    }
}
}
