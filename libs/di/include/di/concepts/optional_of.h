#pragma once

#include <di/concepts/optional.h>
#include <di/meta/core.h>
#include <di/meta/optional_value.h>

namespace di::concepts {
template<typename Opt, typename T>
concept OptionalOf = Optional<Opt> && SameAs<T, meta::OptionalValue<Opt>>;
;
}
