#pragma once

#include <di/meta/decay.h>
#include <di/util/forward.h>
#include <di/vocab/tuple/tuple.h>

namespace di::vocab {
template<typename... Args>
constexpr auto make_decayed_tuple(Args&&... args) {
    return Tuple<meta::Decay<Args>...>(util::forward<Args>(args)...);
}
}
