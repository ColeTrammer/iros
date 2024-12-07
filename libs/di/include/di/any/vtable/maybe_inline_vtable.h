#pragma once

#include "di/any/vtable/inline_vtable.h"
#include "di/any/vtable/out_of_line_vtable.h"
#include "di/meta/algorithm.h"

namespace di::any {
template<size_t threshold>
struct MaybeInlineVTable {
    template<size_t N>
    constexpr static bool store_out_of_line = N > threshold;

    template<concepts::Interface Interface>
    using Invoke =
        meta::Invoke<meta::Conditional<store_out_of_line<meta::Size<Interface>>, OutOfLineVTable, InlineVTable>,
                     Interface>;
};
}
