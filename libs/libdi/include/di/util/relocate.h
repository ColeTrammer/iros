#pragma once

#include <di/util/destroy_at.h>
#include <di/util/meta/decay.h>
#include <di/util/move.h>

namespace di::util {
template<typename T>
constexpr auto relocate(T&& value) {
    auto result = auto(util::move(value));
    util::destroy_at(&value);
    return result;
}
}
