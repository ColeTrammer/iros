#pragma once

#include <di/meta/remove_reference.h>
#include <di/meta/type_constant.h>
#include <di/meta/type_identity.h>

namespace di::meta {
namespace detail {
    template<typename T>
    struct AddPointerHelper : TypeConstant<T> {};

    template<typename T>
    requires(requires { typename TypeIdentity<RemoveReference<T> *>; })
    struct AddPointerHelper<T> : TypeConstant<RemoveReference<T> *> {};
}

// This is a helper template which will convert reference types into their
// corresponding pointer type, while also working for non-references.
// For types which cannot be made into a pointer (function types), this does nothing.
template<typename T>
using AddPointer = detail::AddPointerHelper<T>::Type;
}
