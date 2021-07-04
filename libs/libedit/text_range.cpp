#include <edit/text_range.h>

namespace Edit {
bool TextRange::includes(int line_index, int index_into_line) const {
    return !starts_after(line_index, index_into_line) && !ends_before(line_index, index_into_line);
}

bool TextRange::starts_after(int line_index, int index_into_line) const {
    if (line_index < start_line_index()) {
        return true;
    }

    if (line_index == start_line_index()) {
        return index_into_line < start_index_into_line();
    }

    return false;
}

bool TextRange::ends_before(int line_index, int index_into_line) const {
    if (line_index > end_line_index()) {
        return true;
    }

    if (line_index == end_line_index()) {
        return index_into_line > end_index_into_line();
    }

    return false;
}
}
