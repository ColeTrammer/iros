#pragma once

#include <di/concepts/copy_constructible.h>
#include <di/concepts/maybe_fallible.h>
#include <di/function/tag_invoke.h>
#include <di/util/create.h>

namespace di::util {
namespace detail {
    struct CloneFunction;

    template<typename T>
    concept CustomClonable = concepts::TagInvocable<CloneFunction, T const&> &&
                             concepts::MaybeFallible<meta::TagInvokeResult<CloneFunction, T const&>, T>;

    template<typename T>
    concept MemberClonable = requires(T const& value) {
                                 { value.clone() } -> concepts::MaybeFallible<T>;
                             };

    struct CloneFunction {
        template<typename T>
        requires(concepts::CopyConstructible<T> || CustomClonable<T> || MemberClonable<T> ||
                 concepts::CreatableFrom<T, T const&>)
        constexpr auto operator()(T const& value) const {
            if constexpr (concepts::CopyConstructible<T>) {
                return value;
            } else if constexpr (CustomClonable<T>) {
                return function::tag_invoke(*this, value);
            } else if constexpr (MemberClonable<T>) {
                return value.clone();
            } else {
                return util::create<T>(value);
            }
        }
    };
}

constexpr inline auto clone = detail::CloneFunction {};
}

namespace di::concepts {
template<typename T>
concept Clonable = requires(T const& value) { util::clone(value); };
}
