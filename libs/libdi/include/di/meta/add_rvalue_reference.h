#pragma once

#include <di/meta/type_constant.h>
#include <di/meta/type_identity.h>

namespace di::meta {
namespace detail {
    template<typename T>
    struct AddRValueReferenceHelper : TypeConstant<T> {};

    template<typename T>
    requires(requires { typename TypeIdentity<T&&>; })
    struct AddRValueReferenceHelper<T> : TypeConstant<T&&> {};
}

template<typename T>
using AddRValueReference = detail::AddRValueReferenceHelper<T>::Type;
}
