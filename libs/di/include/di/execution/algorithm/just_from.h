#pragma once

#include "di/execution/algorithm/just.h"
#include "di/execution/algorithm/then.h"
#include "di/function/invoke.h"
#include "di/meta/util.h"

namespace di::execution {
namespace just_from_ns {
    struct Function {
        template<concepts::MovableValue Fun>
        requires(concepts::Invocable<Fun>)
        auto operator()(Fun&& function) const {
            return just() | then(util::forward<Fun>(function));
        }
    };
}

/// @brief Creates a sender from a function.
///
/// @param function The function which is invoked to send a value.
///
/// @return A sender which invokes the provided function to send a value.
///
/// This function is like execution::just() but does does not take a value. Instead, it takes a function which is called
/// when the operation is started. This is a shorthand for:
///
/// ```cpp
///  execution::just() | execution::then(function)
/// ```
///
/// @see just
/// @see then
constexpr inline auto just_from = just_from_ns::Function {};
}
