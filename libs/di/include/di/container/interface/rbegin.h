#pragma once

#include <di/container/concepts/iterator.h>
#include <di/container/interface/begin.h>
#include <di/container/interface/end.h>
#include <di/container/iterator/reverse_iterator.h>
#include <di/function/tag_invoke.h>
#include <di/meta/core.h>
#include <di/meta/language.h>
#include <di/meta/util.h>
#include <di/util/forward.h>

namespace di::container {
struct RBeginFunction;

namespace detail {
    template<typename T>
    concept CustomRBegin = concepts::TagInvocable<RBeginFunction, T> &&
                           concepts::Iterator<meta::Decay<meta::TagInvokeResult<RBeginFunction, T>>>;

    template<typename T>
    concept MemberRBegin = requires(T&& container) {
        { util::forward<T>(container).rbegin() } -> concepts::Iterator;
    };

    template<typename T>
    concept ReverseIteratorRBegin = requires(T&& container) {
        { container::begin(util::forward<T>(container)) } -> concepts::BidirectionalIterator;
        {
            container::end(util::forward<T>(container))
        } -> concepts::SameAs<decltype(container::begin(util::forward<T>(container)))>;
    };
}

struct RBeginFunction {
    template<typename T>
    requires(enable_borrowed_container(types::in_place_type<meta::RemoveCV<T>>) &&
             (detail::CustomRBegin<T> || detail::MemberRBegin<T> || detail::ReverseIteratorRBegin<T>) )
    constexpr concepts::Iterator auto operator()(T&& container) const {
        if constexpr (detail::CustomRBegin<T>) {
            return function::tag_invoke(*this, util::forward<T>(container));
        } else if constexpr (detail::MemberRBegin<T>) {
            return util::forward<T>(container).rbegin();
        } else {
            return make_reverse_iterator(container::begin(util::forward<T>(container)));
        }
    }
};

constexpr inline auto rbegin = RBeginFunction {};
}

namespace di {
using container::rbegin;
}
