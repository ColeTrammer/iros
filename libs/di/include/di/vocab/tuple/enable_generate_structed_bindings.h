
#pragma once

#include <di/concepts/same_as.h>
#include <di/function/tag_invoke.h>
#include <di/meta/make_index_sequence.h>
#include <di/meta/remove_cv.h>
#include <di/meta/remove_reference.h>
#include <di/types/in_place_type.h>
#include <di/util/forward.h>
#include <di/vocab/tuple/std_structed_binding.h>
#include <di/vocab/tuple/tuple_like.h>
#include <di/vocab/tuple/tuple_size.h>

namespace di::vocab {
constexpr inline struct EnableGenerateStructedBindingsFunction {
    template<typename T>
    constexpr auto operator()(types::InPlaceType<T> x) const {
        if constexpr (concepts::TagInvocableTo<EnableGenerateStructedBindingsFunction, bool, decltype(x)>) {
            return function::tag_invoke(*this, x);
        } else {
            return false;
        }
    }
} enable_generate_structed_bindings;
}

namespace di::concepts::detail {
template<typename T, typename Indices>
struct CanStructuredBindHelper;

template<typename T, types::size_t index>
concept HasMemberGet = requires(T value) { util::forward<T>(value).template get<index>(); };

template<typename T, types::size_t... indices>
struct CanStructuredBindHelper<T, meta::ListV<indices...>> {
    constexpr static bool value = (HasMemberGet<T, indices> && ...);
};

template<typename T>
concept CanStructuredBind = SameAs<T, meta::RemoveReference<T>> && TupleLike<T> &&
                            vocab::enable_generate_structed_bindings(types::in_place_type<meta::RemoveCV<T>>) &&
                            CanStructuredBindHelper<T, meta::MakeIndexSequence<meta::TupleSize<T>>>::value;
}

// Include the generate structed bindings here, so
// that any type which does enable structed bindings will have those
// declarations immediately.
namespace std {
template<di::concepts::detail::CanStructuredBind T>
struct tuple_size<T> {
    constexpr static di::types::size_t value = di::meta::TupleSize<T>;
};

template<di::types::size_t index, di::concepts::detail::CanStructuredBind T>
struct tuple_element<index, T> {
    using type = di::meta::TupleElement<T, index>;
};
}
