#include <liim/container/algorithm/fold.h>
#include <liim/container/view/transform.h>
#include <liim/utf8_view.h>
#include <unicode/east_asian_width.h>
#include <unicode/terminal_width.h>

namespace Unicode {
size_t terminal_code_point_width(uint32_t code_point) {
    // FIXME: should this ever return 0?
    // FIXME: technically, ambigous characters should have width 2 if we know that this text is east asian.
    // FIXME: east asian width is not technically the correct property to consult, although it is the defacto standard.
    auto east_asian_width_value = east_asian_width(code_point);
    switch (east_asian_width_value) {
        case EastAsianWidth::W:
        case EastAsianWidth::F:
            return 2;
        default:
            return 1;
    }
}

size_t terminal_width(Utf8View view) {
    return Alg::fold(transform(view, terminal_code_point_width), 0, Plus {});
}
}
