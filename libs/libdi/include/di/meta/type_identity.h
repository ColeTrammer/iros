#pragma once

#include <di/meta/type_constant.h>

namespace di::meta {
namespace detail {
    template<typename T>
    struct TypeIdentityHelper : meta::TypeConstant<T> {};
}

// This is a helper template to prevent C++ from deducing the type of template argument.
// This will force users/callers to specify the template types inside the <> brackets.
template<typename T>
using TypeIdentity = detail::TypeIdentityHelper<T>::Type;
}
