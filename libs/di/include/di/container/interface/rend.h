#pragma once

#include <di/concepts/language_array.h>
#include <di/container/concepts/iterator.h>
#include <di/container/concepts/sentinel_for.h>
#include <di/container/interface/begin.h>
#include <di/container/interface/end.h>
#include <di/container/interface/rbegin.h>
#include <di/container/iterator/reverse_iterator.h>
#include <di/function/tag_invoke.h>
#include <di/meta/decay.h>
#include <di/meta/remove_reference.h>
#include <di/util/declval.h>
#include <di/util/forward.h>

namespace di::container {
struct REndFunction;

namespace detail {
    template<typename R, typename T>
    concept ValidREndReturn = concepts::SentinelFor<R, decltype(container::rbegin(util::declval<T>()))>;

    template<typename T>
    concept CustomREnd = concepts::TagInvocable<REndFunction, T> &&
                         ValidREndReturn<meta::Decay<meta::TagInvokeResult<REndFunction, T>>, T>;

    template<typename T>
    concept MemberREnd = requires(T&& container) {
        { util::forward<T>(container).rend() } -> ValidREndReturn<T>;
    };

    template<typename T>
    concept ReverseIteratorREnd = requires(T&& container) {
        { container::begin(util::forward<T>(container)) } -> concepts::BidirectionalIterator;
        {
            container::end(util::forward<T>(container))
        } -> concepts::SameAs<decltype(container::begin(util::forward<T>(container)))>;
    };
}

struct REndFunction {
    template<typename T>
    requires(enable_borrowed_container(types::in_place_type<meta::RemoveCV<T>>) &&
             (detail::CustomREnd<T> || detail::MemberREnd<T> || detail::ReverseIteratorREnd<T>) )
    constexpr detail::ValidREndReturn<T> auto operator()(T&& container) const {
        if constexpr (detail::CustomREnd<T>) {
            return function::tag_invoke(*this, util::forward<T>(container));
        } else if constexpr (detail::MemberREnd<T>) {
            return util::forward<T>(container).rend();
        } else {
            return make_reverse_iterator(container::end(util::forward<T>(container)));
        }
    }
};

constexpr inline auto rend = REndFunction {};
}
