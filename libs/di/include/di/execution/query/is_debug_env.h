#pragma once

#include "di/execution/query/forwarding_query.h"
#include "di/function/invoke.h"
#include "di/meta/constexpr.h"

namespace di::execution {
namespace is_debug_env_ns {
    struct Function : ForwardingQuery {
        template<typename Env>
        constexpr auto operator()(Env const& env) const {
            if constexpr (concepts::TagInvocable<Function, Env const&>) {
                static_assert(concepts::ConstexprOf<meta::TagInvokeResult<Function, Env const&>, bool>,
                              "is_debug_env() customizations must return a di::Constexpr<bool> instance.");
                return tag_invoke(*this, env);
            } else {
                return c_<false>;
            }
        }
    };
}

/// @brief Returns whether the given environment is a debug environment.
///
/// @param env The environment to query.
///
/// @return Whether the given environment is a debug environment.
///
/// This is a debug mechanism, which forces the execution::connect or execution::subscribe CPOs to call be valid even if
/// no customization is valid. This is useful for seeing why a customization is not valid, since it will cause a
/// compiler error when trying to call the CPO.
///
/// This mechanism is nice since it does not require manually editing the
/// CPO to remove the constraints, which was the previous debugging mechanism.
///
/// @see connect
/// @see subscribe
/// @see with_debug_env
constexpr inline auto is_debug_env = is_debug_env_ns::Function {};
}

namespace di::concepts {
/// @brief Check the given environment is a debug environment.
///
/// @tparam Env The environment to check.
///
/// @see execution::is_debug_env
template<typename Env>
concept DebugEnv = meta::InvokeResult<decltype(execution::is_debug_env), Env const&>::value;
}
