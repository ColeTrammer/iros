#pragma once

#include <di/concepts/language_function.h>
#include <di/concepts/language_void.h>
#include <di/concepts/reference.h>
#include <di/concepts/unbounded_language_array.h>
#include <di/meta/remove_all_extents.h>
#include <di/util/declval.h>

namespace di::concepts {
namespace detail {
    template<typename T>
    concept DestructibleHelper = requires { util::declval<T&>().~T(); };
}

template<typename T>
concept Destructible = Reference<T> || (!LanguageVoid<T> && !LanguageFunction<T> && !UnboundedLanguageArray<T> &&
                                        detail::DestructibleHelper<meta::RemoveAllExtents<T>>);
}
