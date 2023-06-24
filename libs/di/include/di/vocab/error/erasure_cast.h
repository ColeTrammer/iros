#pragma once

#include <di/meta/trivial.h>
#include <di/util/bit_cast.h>
#include <di/util/compile_time_fail.h>

namespace di::vocab::detail {
template<concepts::TriviallyRelocatable To, concepts::TriviallyRelocatable From>
constexpr To erasure_cast(From const& from) {
    return util::bit_cast<To>(from);
}
}
