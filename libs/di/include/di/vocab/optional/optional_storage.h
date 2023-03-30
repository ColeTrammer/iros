#pragma once

#include <di/concepts/default_constructible.h>
#include <di/concepts/same_as.h>
#include <di/util/as_const.h>
#include <di/util/forward.h>
#include <di/util/move.h>
#include <di/vocab/optional/get_value.h>
#include <di/vocab/optional/is_nullopt.h>
#include <di/vocab/optional/nullopt.h>
#include <di/vocab/optional/set_nullopt.h>
#include <di/vocab/optional/set_value.h>

namespace di::vocab {
template<typename Storage, typename T>
concept OptionalStorage = requires(Storage& storage, T&& value) {
    { is_nullopt(util::as_const(storage)) } -> concepts::SameAs<bool>;
    { set_nullopt(storage) };
    { get_value(storage) };
    { get_value(util::as_const(storage)) };
    { get_value(util::move(storage)) };
    { get_value(util::move(util::as_const(storage))) };
} && concepts::ConstructibleFrom<Storage, NullOpt>;
}
