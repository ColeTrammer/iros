#pragma once

#include <di/container/concepts/forward_iterator.h>
#include <di/container/concepts/sized_sentinel_for.h>
#include <di/container/interface/begin.h>
#include <di/container/interface/end.h>
#include <di/container/meta/container_iterator.h>
#include <di/container/meta/container_sentinel.h>
#include <di/container/meta/iterator_size_type.h>
#include <di/function/tag_invoke.h>
#include <di/meta/core.h>
#include <di/meta/language.h>
#include <di/meta/operations.h>
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

struct SizeFunction : function::pipeline::EnablePipeline {
    template<typename T>
    requires(detail::ArraySize<T> || detail::CustomSize<T> || detail::MemberSize<T> || detail::IteratorSize<T>)
    constexpr auto operator()(T&& container) const -> meta::IteratorSizeType<meta::ContainerIterator<T>> {
        if constexpr (detail::ArraySize<T>) {
            return meta::Extent<meta::RemoveReference<T>>;
        } else if constexpr (detail::CustomSize<T>) {
            return function::tag_invoke(*this, util::forward<T>(container));
        } else if constexpr (detail::MemberSize<T>) {
            return util::forward<T>(container).size();
        } else {
            return end(container) - begin(container);
        }
    }
};

constexpr inline auto size = SizeFunction {};
}

namespace di {
using container::size;
}
