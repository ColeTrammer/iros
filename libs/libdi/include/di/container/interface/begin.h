#pragma once

#include <di/concepts/language_array.h>
#include <di/container/concepts/iterator.h>
#include <di/container/interface/enable_borrowed_container.h>
#include <di/function/tag_invoke.h>
#include <di/meta/decay.h>
#include <di/meta/remove_reference.h>
#include <di/util/forward.h>

namespace di::container {
struct BeginFunction;

namespace detail {
    template<typename T>
    concept ArrayBegin = concepts::LanguageArray<meta::RemoveReference<T>>;

    template<typename T>
    concept CustomBegin = concepts::TagInvocable<BeginFunction, T> &&
                          concepts::Iterator<meta::Decay<meta::TagInvokeResult<BeginFunction, T>>>;

    template<typename T>
    concept MemberBegin = requires(T&& container) {
                              { util::forward<T>(container).begin() } -> concepts::Iterator;
                          };
}

struct BeginFunction {
    template<typename T>
    requires(enable_borrowed_container(types::in_place_type<meta::RemoveCV<T>>) &&
             (detail::ArrayBegin<T> || detail::CustomBegin<T> || detail::MemberBegin<T>) )
    constexpr auto operator()(T&& container) const {
        if constexpr (detail::ArrayBegin<T>) {
            return container + 0;
        } else if constexpr (detail::CustomBegin<T>) {
            return function::tag_invoke(*this, util::forward<T>(container));
        } else {
            return util::forward<T>(container).begin();
        }
    }
};

constexpr inline auto begin = BeginFunction {};
}
