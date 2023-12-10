#pragma once

#include <di/meta/core.h>
#include <di/types/size_t.h>

namespace di::concepts {
template<typename T>
concept FormatArgs = requires(T const& args, size_t index) {
    { args.size() } -> SameAs<size_t>;
    args[index];
};
}
