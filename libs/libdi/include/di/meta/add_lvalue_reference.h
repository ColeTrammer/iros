#pragma once

#include <di/meta/type_constant.h>
#include <di/meta/type_identity.h>

namespace di::meta {
namespace detail {
    template<typename T>
    struct AddLValueReferenceHelper : TypeConstant<T> {};

    template<typename T>
    requires(requires { typename TypeIdentity<T&>; })
    struct AddLValueReferenceHelper<T> : TypeConstant<T&> {};
}

template<typename T>
using AddLValueReference = detail::AddLValueReferenceHelper<T>::Type;
}
