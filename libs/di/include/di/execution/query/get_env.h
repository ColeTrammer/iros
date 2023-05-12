#pragma once

#include <di/concepts/not_same_as.h>
#include <di/execution/concepts/queryable.h>
#include <di/execution/query/forwarding_query.h>
#include <di/execution/types/no_env.h>
#include <di/function/tag_invoke.h>

namespace di::execution {
namespace detail {
    struct GetEnvFunction : ForwardingQuery {
        template<typename T>
        requires(concepts::TagInvocable<GetEnvFunction, T const&>)
        constexpr auto operator()(T const& value) const {
            static_assert(concepts::Queryable<meta::TagInvokeResult<GetEnvFunction, T const&>>,
                          "get_env() must return a Queryable.");
            return function::tag_invoke(*this, value);
        }

        constexpr auto operator()() const;
    };
}

constexpr inline auto get_env = detail::GetEnvFunction {};
}
