#pragma once

#include <di/util/meta/type_constant.h>

namespace di::util::meta {
namespace detail {
    template<typename T>
    struct RemoveConstHelper : TypeConstant<T> {};

    template<typename T>
    struct RemoveConstHelper<T const> : TypeConstant<T> {};
}

template<typename T>
using RemoveConst = detail::RemoveConstHelper<T>::Type;
}
