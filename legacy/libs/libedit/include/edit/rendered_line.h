#pragma once

#include <edit/character_metadata.h>
#include <edit/forward.h>
#include <edit/relative_position.h>
#include <liim/string.h>
#include <liim/vector.h>

namespace Edit {
enum class PositionRangeType {
    Normal,
    InlineBeforeCursor,
    InlineAfterCursor,
};

struct PositionRange {
    RelativePosition start;
    RelativePosition end;
    int start_absolute_col { 0 };
    int index_into_line { 0 };
    CharacterMetadata metadata { 0 };
    int byte_offset_in_rendered_string { 0 };
    int byte_count_in_rendered_string { 0 };
    PositionRangeType type { PositionRangeType::Normal };
};

class RenderedLine {
public:
    RenderedLine() = default;

    Span<const String> rendered_lines() const { return m_rendered_lines.span(); }
    Span<const Vector<PositionRange>> position_ranges() const { return m_position_ranges.span(); }

    bool is_up_to_date() const;
    void invalidate();

    void append_line(String&& string, Vector<PositionRange>&& ranges);
    void update_metadata(Display& display, const TextIndex& this_line_start);

    RelativePosition relative_position_of_index(const TextIndex& index) const;
    int absolute_col_offset_of_index(const TextIndex& index) const;

    TextIndex index_of_absolute_position(const AbsolutePosition& position) const;

    int rendered_line_count() const;

    TextIndex next_index_into_line(const TextIndex& index) const;
    TextIndex prev_index_into_line(const TextIndex& index) const;

    int render(Display& display, const TextIndex& this_line_start, int col_offset, int relative_row_start, int row_in_display);

private:
    enum class RangeFor {
        Text,
        Cursor,
    };

    const PositionRange* range_for_relative_position(const RelativePosition& position) const;
    const PositionRange* range_for_index(const TextIndex& index, RangeFor mode) const;

    Vector<String> m_rendered_lines;
    Vector<Vector<PositionRange>> m_position_ranges;
};
}
