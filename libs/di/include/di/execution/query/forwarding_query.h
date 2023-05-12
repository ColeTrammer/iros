#pragma once

#include <di/concepts/derived_from.h>
#include <di/concepts/same_as.h>
#include <di/function/tag_invoke.h>

namespace di::execution {
struct ForwardingQuery {
    constexpr bool operator()(auto tag) const {
        if constexpr (concepts::TagInvocable<ForwardingQuery, decltype(tag)>) {
            static_assert(concepts::SameAs<bool, meta::TagInvokeResult<ForwardingQuery, decltype(tag)>>,
                          "ForwardingQuery must return bool");
            return function::tag_invoke(*this, tag);
        } else if (concepts::DerivedFrom<decltype(tag), ForwardingQuery>) {
            return true;
        } else {
            return false;
        }
    }
};

constexpr inline auto forwarding_query = ForwardingQuery {};
}
