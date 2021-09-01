#pragma once

#include <edit/character_metadata.h>
#include <edit/position.h>
#include <liim/string.h>
#include <liim/vector.h>

namespace Edit {
enum class PositionRangeType {
    Normal,
    InlineBeforeCursor,
    InlineAfterCursor,
};

struct PositionRange {
    Position start;
    Position end;
    int start_absolute_col { 0 };
    int index_into_line { 0 };
    CharacterMetadata metadata { 0 };
    int byte_count_in_rendered_string { 0 };
    PositionRangeType type { PositionRangeType::Normal };
};

struct RenderedLine {
    Vector<String> rendered_lines;
    Vector<Vector<PositionRange>> position_ranges;
};
}
