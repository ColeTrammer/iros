#include <assert.h>
#include <edit/character_metadata.h>
#include <edit/display.h>
#include <edit/document.h>
#include <edit/line.h>
#include <edit/position.h>
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

void Line::overwrite(Document& document, Line&& line, OverwriteFrom mode) {
    auto old_length = this->length();
    auto delta_length = line.length() - old_length;
    this->m_contents = move(line.contents());
    if (mode == OverwriteFrom::None) {
        return;
    }

    auto start_index = mode == OverwriteFrom::LineStart ? 0 : min(old_length, length());
    if (delta_length < 0) {
        document.emit<DeleteFromLine>(document.index_of_line(*this), start_index, -delta_length);
    } else if (delta_length > 0) {
        document.emit<AddToLine>(document.index_of_line(*this), start_index, delta_length);
    } else {
        document.invalidate_rendered_contents(*this);
    }
}

Position Line::relative_position_of_index(const Document& document, Display& display, int index) const {
    auto& info = compute_rendered_contents(document, display);

    auto* range = range_for_index_into_line(document, display, index, RangeFor::Cursor);
    if (!range) {
        assert(!info.position_ranges.empty());
        return info.position_ranges.last().last().end;
    }
    return range->start;
}

int Line::absoulte_col_offset_of_index(const Document& document, Display& display, int index) const {
    auto* range = range_for_index_into_line(document, display, index, RangeFor::Text);
    if (!range) {
        return 0;
    }

    return range->start_absolute_col;
}

const PositionRange* Line::range_for_index_into_line(const Document& document, Display& display, int index_into_line, RangeFor mode) const {
    auto& info = compute_rendered_contents(document, display);

    for (auto& position_ranges : info.position_ranges) {
        for (auto& range : position_ranges) {
            if (range.index_into_line == index_into_line) {
                if (mode == RangeFor::Text && range.type == PositionRangeType::Normal) {
                    return &range;
                }
                if (mode == RangeFor::Cursor &&
                    (range.type == PositionRangeType::Normal || range.type == PositionRangeType::InlineAfterCursor)) {
                    return &range;
                }
            }
        }
    }
    return nullptr;
}

const PositionRange* Line::range_for_relative_position(const Document& document, Display& display, const Position& position) const {
    auto& info = compute_rendered_contents(document, display);

    if (position.row < 0 || position.row >= info.position_ranges.size()) {
        return nullptr;
    }

    for (auto& range : info.position_ranges[position.row]) {
        if (position >= range.start && position < range.end) {
            return &range;
        }
    }
    return nullptr;
}

int Line::index_of_relative_position(const Document& document, Display& display, const Position& position) const {
    auto* range = range_for_relative_position(document, display, position);
    if (!range) {
        return length();
    }
    return range->index_into_line;
}

int Line::absolute_row_position(const Document& document, Display& display) const {
    if (!display.word_wrap_enabled()) {
        return document.index_of_line(*this);
    }

    int absolute_row = 0;
    for (int i = 0; i < document.num_lines(); i++) {
        if (this == &document.line_at_index(i)) {
            return absolute_row;
        }
        absolute_row += document.line_at_index(i).rendered_line_count(document, display);
    }
    assert(false);
}

int Line::next_index_into_line(const Document& document, Display& display, int index) const {
    assert(index != length());

    auto& info = compute_rendered_contents(document, display);
    auto* range = range_for_index_into_line(document, display, index, RangeFor::Cursor);
    bool last_in_row = &info.position_ranges[range->end.row].last() == range;

    auto next_position = last_in_row ? Position { range->end.row + 1, 0 } : range->end;
    return index_of_relative_position(document, display, next_position);
}

int Line::prev_index_into_line(const Document& document, Display& display, int index) const {
    assert(index != 0);

    auto& info = compute_rendered_contents(document, display);
    auto* range = range_for_index_into_line(document, display, index, RangeFor::Cursor);
    bool first_in_row = &info.position_ranges[range->start.row].first() == range;

    auto prev_position = first_in_row ? Position { info.position_ranges[range->start.row - 1].last().start }
                                      : Position { range->start.row, range->start.col - 1 };
    return index_of_relative_position(document, display, prev_position);
}

int Line::rendered_line_count(const Document& document, Display& display) const {
    auto& info = compute_rendered_contents(document, display);
    return info.rendered_lines.size();
}

int Line::max_col_in_relative_row(const Document& document, Display& display, int row) const {
    auto& info = compute_rendered_contents(document, display);
    return info.position_ranges[row].last().end.col;
}

void Line::insert_char_at(Document& document, int position, char c) {
    m_contents.insert(c, position);
    document.emit<AddToLine>(document.index_of_line(*this), position, 1);
}

void Line::remove_char_at(Document& document, int position) {
    m_contents.remove_index(position);
    document.emit<DeleteFromLine>(document.index_of_line(*this), position, 1);
}

void Line::combine_line(Document&, Line& line) {
    m_contents += line.contents();
}

void Line::search(const Document& document, const String& text, TextRangeCollection& results) const {
    auto line_index = document.index_of_line(*this);
    int index_into_line = 0;
    for (;;) {
        const char* match = strstr(m_contents.string() + index_into_line, text.string());
        if (!match) {
            break;
        }

        index_into_line = match - m_contents.string();
        results.add({ { line_index, index_into_line },
                      { line_index, index_into_line + static_cast<int>(text.size()) },
                      { CharacterMetadata::Flags::Highlighted } });
        index_into_line += text.size();
    }
}

void Line::invalidate_rendered_contents(const Document& document, Display& display) const {
    display.invalidate_line(document.index_of_line(*this));
}

const RenderedLine& Line::compute_rendered_contents(const Document& document, Display& display) const {
    auto& rendered_line = display.rendered_line_at_index(document.index_of_line(*this));
    if (!rendered_line.rendered_lines.empty()) {
        return rendered_line;
    }

    rendered_line = display.compose_line(*this);
    return rendered_line;
}

int Line::render(const Document& document, Display& display, int col_offset, int relative_row_start, int row_in_display) const {
    auto& info = compute_rendered_contents(document, display);

    auto row_count = rendered_line_count(document, display);
    for (int row = relative_row_start; row + row_in_display - relative_row_start < display.rows() && row < row_count; row++) {
        auto& position_ranges = info.position_ranges[row];
        int range_index = 0;

        auto& rendered_line = info.rendered_lines[row];
        auto metadata_vector = Vector<CharacterMetadata>(rendered_line.size());
        for (; range_index < position_ranges.size(); range_index++) {
            auto& current_range = position_ranges[range_index];
            for (int i = 0; i < current_range.byte_count_in_rendered_string; i++) {
                metadata_vector.add(current_range.metadata);
            }
        }

        assert(rendered_line.size() == static_cast<size_t>(metadata_vector.size()));
        display.output_line(row + row_in_display - relative_row_start, col_offset, rendered_line.view(), metadata_vector);
    }

    return row_count - relative_row_start;
}
}
