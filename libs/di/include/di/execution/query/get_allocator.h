#pragma once

#include <di/container/allocator/allocator.h>
#include <di/container/allocator/fallible_allocator.h>
#include <di/container/allocator/infallible_allocator.h>
#include <di/execution/query/forwarding_query.h>
#include <di/function/tag_invoke.h>
#include <di/platform/prelude.h>
#include <di/util/as_const.h>

namespace di::execution {
namespace detail {
    struct GetAllocatorFunction : ForwardingQuery {
        template<typename T>
        constexpr auto operator()(T&& value) const -> concepts::Allocator auto {
            if constexpr (concepts::TagInvocable<GetAllocatorFunction, T const&>) {
                return function::tag_invoke(*this, util::as_const(value));
            } else {
                return platform::DefaultAllocator {};
            }
        }

        constexpr auto operator()() const;
    };
}

constexpr inline auto get_allocator = detail::GetAllocatorFunction {};
}

namespace di::meta {
template<typename T>
using AllocatorOf = meta::RemoveCVRef<decltype(execution::get_allocator(util::declval<T>()))>;
}
