#pragma once

#include <di/meta/type_constant.h>

namespace di::meta {
namespace detail {
    template<typename T>
    struct RemoveConstHelper : TypeConstant<T> {};

    template<typename T>
    struct RemoveConstHelper<T const> : TypeConstant<T> {};
}

template<typename T>
using RemoveConst = detail::RemoveConstHelper<T>::Type;
}
