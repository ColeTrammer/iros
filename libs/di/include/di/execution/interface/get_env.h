#pragma once

#include <di/concepts/not_same_as.h>
#include <di/execution/concepts/queryable.h>
#include <di/execution/query/forwarding_query.h>
#include <di/execution/types/empty_env.h>
#include <di/function/tag_invoke.h>

namespace di::execution {
namespace detail {
    struct GetEnvFunction {
        template<typename T>
        constexpr decltype(auto) operator()(T const& value) const {
            if constexpr (concepts::TagInvocable<GetEnvFunction, T const&>) {
                static_assert(concepts::Queryable<meta::TagInvokeResult<GetEnvFunction, T const&>>,
                              "get_env() must return a Queryable.");
                return function::tag_invoke(*this, value);
            } else {
                return types::EmptyEnv {};
            }
        }

        constexpr auto operator()() const;
    };
}

constexpr inline auto get_env = detail::GetEnvFunction {};
}
