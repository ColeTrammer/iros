#pragma once

#include <di/util/meta/type_constant.h>
#include <di/util/meta/type_identity.h>

namespace di::util::meta {
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
