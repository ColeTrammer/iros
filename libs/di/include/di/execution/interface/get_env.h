#pragma once

#include <di/execution/concepts/queryable.h>
#include <di/execution/query/forwarding_query.h>
#include <di/execution/types/empty_env.h>
#include <di/function/tag_invoke.h>
#include <di/meta/util.h>

namespace di::execution {
namespace detail {
    struct GetEnvFunction {
        template<typename T>
        constexpr auto operator()(T const& value) const -> decltype(auto) {
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
