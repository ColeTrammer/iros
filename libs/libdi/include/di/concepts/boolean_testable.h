#pragma once

#include <di/concepts/convertible_to.h>
#include <di/util/forward.h>

namespace di::concepts {
namespace detail {
    template<typename T>
    concept BooleanTestableImpl = ConvertibleTo<T, bool>;
}

template<typename T>
concept BooleanTestable = detail::BooleanTestableImpl<T> && requires(T&& value) {
                                                                {
                                                                    !util::forward<T>(value)
                                                                    } -> detail::BooleanTestableImpl;
                                                            };
}