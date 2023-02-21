#pragma once

#include <di/concepts/convertible_to.h>
#include <di/concepts/same_as.h>
#include <di/meta/common_reference.h>

namespace di::concepts {
template<typename T, typename U>
concept CommonReferenceWith =
    SameAs<meta::CommonReference<T, U>, meta::CommonReference<U, T>> && ConvertibleTo<T, meta::CommonReference<T, U>> &&
    ConvertibleTo<U, meta::CommonReference<T, U>>;
}
