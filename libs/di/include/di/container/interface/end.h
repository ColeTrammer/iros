#pragma once

#include <di/container/concepts/iterator.h>
#include <di/container/concepts/sentinel_for.h>
#include <di/container/interface/enable_borrowed_container.h>
#include <di/container/meta/container_iterator.h>
#include <di/function/tag_invoke.h>
#include <di/meta/core.h>
#include <di/meta/language.h>
#include <di/meta/util.h>
#include <di/types/size_t.h>
#include <di/util/forward.h>

namespace di::container {
struct EndFunction;

namespace detail {
    template<typename T>
    concept ArrayEnd = concepts::BoundedLanguageArray<meta::RemoveReference<T>>;

    template<typename T>
    concept CustomEnd =
        concepts::TagInvocable<EndFunction, T> &&
        concepts::SentinelFor<meta::Decay<meta::TagInvokeResult<EndFunction, T>>, meta::ContainerIterator<T>>;

    template<typename T>
    concept MemberEnd = requires(T&& container) {
        { util::forward<T>(container).end() } -> concepts::SentinelFor<meta::ContainerIterator<T>>;
    };
}

struct EndFunction : function::pipeline::EnablePipeline {
    template<typename T>
    requires(enable_borrowed_container(types::in_place_type<meta::RemoveCV<T>>) &&
             (detail::ArrayEnd<T> || detail::CustomEnd<T> || detail::MemberEnd<T>) )
    constexpr auto operator()(T&& container) const {
        if constexpr (detail::ArrayEnd<T>) {
            return container + meta::Extent<meta::RemoveReference<T>>;
        } else if constexpr (detail::CustomEnd<T>) {
            return function::tag_invoke(*this, util::forward<T>(container));
        } else {
            return util::forward<T>(container).end();
        }
    }
};

constexpr inline auto end = EndFunction {};
}

namespace di {
using container::end;
}
