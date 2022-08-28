#pragma once

#include <di/concepts/bounded_language_array.h>
#include <di/container/concepts/iterator.h>
#include <di/container/concepts/sentinel_for.h>
#include <di/container/meta/container_iterator.h>
#include <di/meta/decay.h>
#include <di/meta/extent.h>
#include <di/meta/remove_reference.h>
#include <di/types/size_t.h>
#include <di/util/forward.h>
#include <di/container/interface/enable_borrowed_container.h>
#include <di/util/tag_invoke.h>

namespace di::container {
struct EndFunction;

namespace detail {
    template<typename T>
    concept ArrayEnd = concepts::BoundedLanguageArray<meta::RemoveReference<T>>;

    template<typename T>
    concept CustomEnd = concepts::TagInvocable<EndFunction, T> &&
                        concepts::SentinelFor<meta::Decay<meta::TagInvokeResult<EndFunction, T>>, meta::ContainerIterator<T>>;

    template<typename T>
    concept MemberEnd = requires(T&& container) {
                            { util::forward<T>(container).end() } -> concepts::SentinelFor<meta::ContainerIterator<T>>;
                        };
}

struct EndFunction {
    template<typename T>
    requires(enable_borrowed_container(types::in_place_type<T>) && (detail::ArrayEnd<T> || detail::CustomEnd<T> || detail::MemberEnd<T>))
    constexpr auto operator()(T&& container) const {
        if constexpr (detail::ArrayEnd<T>) {
            return container + meta::Extent<meta::RemoveReference<T>>;
        } else if constexpr (detail::CustomEnd<T>) {
            return util::tag_invoke(*this, util::forward<T>(container));
        } else {
            return util::forward<T>(container).end();
        }
    }
};

constexpr inline auto end = EndFunction {};
}
