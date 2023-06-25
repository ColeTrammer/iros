#pragma once

#include <di/function/tag_invoke.h>
#include <di/meta/algorithm.h>
#include <di/meta/core.h>
#include <di/reflect/atom.h>
#include <di/reflect/enumerator.h>
#include <di/reflect/field.h>
#include <di/types/prelude.h>
#include <di/vocab/tuple/tuple_like.h>

namespace di::concepts {
template<typename T>
concept ReflectionValue = concepts::InstanceOf<T, reflection::Fields> ||
                          concepts::InstanceOf<T, reflection::Enumerators> || concepts::InstanceOf<T, reflection::Atom>;
}

namespace di::reflection {
namespace detail {
    struct ReflectFunction {
        template<typename T, typename U = meta::RemoveCVRef<T>>
        requires(concepts::TagInvocable<ReflectFunction, InPlaceType<U>>)
        constexpr decltype(auto) operator()(InPlaceType<T>) const {
            using R = meta::TagInvokeResult<ReflectFunction, InPlaceType<U>>;
            static_assert(concepts::ReflectionValue<R>, "Reflect function must return fields or an atom");
            return function::tag_invoke(*this, in_place_type<U>);
        }

        template<typename T, typename U = meta::RemoveCVRef<T>>
        requires(!concepts::TagInvocable<ReflectFunction, InPlaceType<U>> &&
                 (concepts::SameAs<U, bool> || concepts::Integer<U> || concepts::detail::ConstantString<U> ||
                  concepts::Container<U>) )
        constexpr decltype(auto) operator()(InPlaceType<T>) const {
            return Atom<U> {};
        }

        template<typename T, typename U = meta::RemoveCVRef<T>>
        requires(!concepts::InstanceOf<U, InPlaceType> &&
                 requires { (util::declval<ReflectFunction const&>())(in_place_type<U>); })
        constexpr decltype(auto) operator()(T&&) const {
            return (*this)(in_place_type<U>);
        }
    };
}

constexpr inline auto reflect = detail::ReflectFunction {};
}

namespace di::concepts {
template<typename T>
concept Reflectable = requires {
    { reflection::reflect(util::declval<T>()) };
};

template<typename T>
concept ReflectableToAtom = requires {
    { reflection::reflect(util::declval<T>()) } -> InstanceOf<reflection::Atom>;
};

template<typename T>
concept ReflectableToFields = requires {
    { reflection::reflect(util::declval<T>()) } -> InstanceOf<reflection::Fields>;
};

template<typename T>
concept ReflectableToEnumerators = requires {
    { reflection::reflect(util::declval<T>()) } -> InstanceOf<reflection::Enumerators>;
};
}

namespace di::meta {
template<concepts::Reflectable T>
using Reflect = decltype(reflection::reflect(in_place_type<T>));
}

namespace di {
using concepts::Reflectable;
using concepts::ReflectableToAtom;
using concepts::ReflectableToFields;
using concepts::ReflectionValue;
using meta::Reflect;
using reflection::reflect;
}
