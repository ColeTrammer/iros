#pragma once

#include <di/concepts/bounded_language_array.h>
#include <di/concepts/implicitly_convertible_to.h>
#include <di/concepts/same_as.h>
#include <di/container/concepts/forward_iterator.h>
#include <di/container/concepts/sized_sentinel_for.h>
#include <di/container/interface/begin.h>
#include <di/container/interface/end.h>
#include <di/container/meta/container_iterator.h>
#include <di/container/meta/container_sentinel.h>
#include <di/container/meta/iterator_size_type.h>
#include <di/function/tag_invoke.h>
#include <di/meta/extent.h>
#include <di/meta/remove_reference.h>
#include <di/types/size_t.h>

namespace di::container {
struct SizeFunction;

namespace detail {
    template<typename T>
    concept ArraySize = concepts::BoundedLanguageArray<meta::RemoveReference<T>>;

    template<typename T>
    concept CustomSize = concepts::TagInvocableTo<SizeFunction, meta::IteratorSizeType<meta::ContainerIterator<T>>, T>;

    template<typename T>
    concept MemberSize = requires(T&& container) {
        {
            util::forward<T>(container).size()
        } -> concepts::ImplicitlyConvertibleTo<meta::IteratorSizeType<meta::ContainerIterator<T>>>;
    };

    template<typename T>
    concept IteratorSize = concepts::ForwardIterator<meta::ContainerIterator<T>> &&
                           concepts::SizedSentinelFor<meta::ContainerSentinel<T>, meta::ContainerIterator<T>>;
}

struct SizeFunction {
    template<typename T>
    requires(detail::ArraySize<T> || detail::CustomSize<T> || detail::MemberSize<T> || detail::IteratorSize<T>)
    constexpr meta::IteratorSizeType<meta::ContainerIterator<T>> operator()(T&& container) const {
        if constexpr (detail::ArraySize<T>) {
            return meta::Extent<meta::RemoveReference<T>>;
        } else if constexpr (detail::CustomSize<T>) {
            return function::tag_invoke(*this, util::forward<T>(container));
        } else if constexpr (detail::MemberSize<T>) {
            return util::forward<T>(container).size();
        } else {
            return end(util::forward<T>(container)) - begin(util::forward<T>(container));
        }
    }
};

constexpr inline auto size = SizeFunction {};
}
