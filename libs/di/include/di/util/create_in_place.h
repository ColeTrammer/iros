#pragma once

#include "di/function/tag_invoke.h"
#include "di/meta/operations.h"
#include "di/meta/vocab.h"
#include "di/types/prelude.h"
#include "di/util/forward.h"

namespace di::util {
namespace detail {
    struct CreateInPlaceFunction;

    template<typename T, typename... Args>
    concept CustomCreatable =
        concepts::TagInvocable<CreateInPlaceFunction, InPlaceType<T>, Args...> &&
        concepts::MaybeFallible<meta::TagInvokeResult<CreateInPlaceFunction, InPlaceType<T>, Args...>, T>;

    template<typename T, typename... Args>
    concept StaticCreatable = requires(Args&&... args) {
        { T::create(util::forward<Args>(args)...) } -> concepts::MaybeFallible<T>;
    };

    struct CreateInPlaceFunction {
        template<typename T, typename... Args>
        requires(concepts::ConstructibleFrom<T, Args...> || CustomCreatable<T, Args...> || StaticCreatable<T, Args...>)
        constexpr auto operator()(InPlaceType<T>, Args&&... args) const {
            if constexpr (concepts::ConstructibleFrom<T, Args...>) {
                return T(util::forward<Args>(args)...);
            } else if constexpr (CustomCreatable<T, Args...>) {
                return function::tag_invoke(*this, in_place_type<T>, util::forward<Args>(args)...);
            } else {
                return T::create(util::forward<Args>(args)...);
            }
        }
    };
}

constexpr inline auto create_in_place = detail::CreateInPlaceFunction {};
}

namespace di {
using util::create_in_place;
}
