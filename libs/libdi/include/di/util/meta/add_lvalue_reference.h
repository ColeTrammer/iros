#pragma once

#include <di/util/meta/type_constant.h>
#include <di/util/meta/type_identity.h>

namespace di::util::meta {
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
