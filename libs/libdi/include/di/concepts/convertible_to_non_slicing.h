#pragma once

#include <di/concepts/convertible_to.h>
#include <di/concepts/pointer.h>
#include <di/meta/decay.h>
#include <di/meta/remove_pointer.h>

namespace di::concepts {
// This concept requires that the conversion from From to To would not
// result in converting a derived type to a base type. This is useful
// to prevent slicing when treating pointers as iterators, since an
// Cat[] can not be viewed the same as an Animal[].
template<typename From, typename To>
concept ConvertibleToNonSlicing =
    ConvertibleTo<From, To> &&
    (!Pointer<meta::Decay<From>> || !Pointer<meta::Decay<To>> ||
     ImplicitlyConvertibleTo<meta::RemovePointer<From> (*)[], meta::RemovePointer<To> (*)[]>);
}
