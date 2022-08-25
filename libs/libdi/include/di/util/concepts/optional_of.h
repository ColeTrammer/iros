#pragma once

#include <di/util/concepts/optional.h>
#include <di/util/concepts/same_as.h>
#include <di/util/meta/optional_value.h>

namespace di::util::concepts {
template<typename Opt, typename T>
concept OptionalOf = Optional<Opt> && SameAs<T, meta::OptionalValue<Opt>>;
;
}
