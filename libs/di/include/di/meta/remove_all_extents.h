#pragma once

#include <di/concepts/language_array.h>
#include <di/meta/remove_extent.h>

namespace di::meta {
namespace detail {
    template<typename T>
    struct RemoveAllExtentsHelper : TypeConstant<T> {};

    template<concepts::LanguageArray T>
    struct RemoveAllExtentsHelper<T> : RemoveAllExtentsHelper<RemoveExtent<T>> {};
}

template<typename T>
using RemoveAllExtents = detail::RemoveAllExtentsHelper<T>::Type;
}
