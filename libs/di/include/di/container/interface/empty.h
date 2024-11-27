#pragma once

#include <di/container/concepts/forward_iterator.h>
#include <di/container/interface/begin.h>
#include <di/container/interface/end.h>
#include <di/container/interface/size.h>
#include <di/container/meta/container_iterator.h>
#include <di/function/tag_invoke.h>

namespace di::container {
struct EmptyFunction;

namespace detail {
    template<typename T>
    concept CustomEmpty = concepts::TagInvocableTo<EmptyFunction, bool, T>;

    template<typename T>
    concept MemberEmpty = requires(T&& container) { static_cast<bool>(container.empty()); };

    template<typename T>
    concept SizeEmpty = requires(T&& container) { static_cast<bool>(container::size(container) == 0); };

    template<typename T>
    concept IteratorEmpty = requires(T&& container) {
        static_cast<bool>(container::begin(container) == container::end(container));
    } && concepts::ForwardIterator<meta::ContainerIterator<T>>;
}

struct EmptyFunction : function::pipeline::EnablePipeline {
    template<typename T>
    requires(detail::CustomEmpty<T> || detail::MemberEmpty<T> || detail::SizeEmpty<T> || detail::IteratorEmpty<T>)
    constexpr auto operator()(T&& container) const -> bool {
        if constexpr (detail::CustomEmpty<T>) {
            return function::tag_invoke(*this, container);
        } else if constexpr (detail::MemberEmpty<T>) {
            return bool(container.empty());
        } else if constexpr (detail::SizeEmpty<T>) {
            return container::size(container) == 0;
        } else {
            return bool(container::begin(container) == container::end(container));
        }
    }
};

constexpr inline auto empty = EmptyFunction {};
}

namespace di {
using container::empty;
}
