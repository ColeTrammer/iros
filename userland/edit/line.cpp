#include "line.h"

LineSplitResult Line::split_at(int position) {
    StringView first = StringView(contents().string(), contents().string() + position - 1);
    StringView second = StringView(contents().string() + position);

    return { Line(first), Line(second) };
}

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