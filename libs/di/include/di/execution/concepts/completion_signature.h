#pragma once

#include "di/execution/receiver/set_error.h"
#include "di/execution/receiver/set_stopped.h"
#include "di/execution/receiver/set_value.h"

namespace di::concepts {
namespace detail {
    template<typename T>
    constexpr inline bool is_completion_signature = false;

    template<typename... Args>
    constexpr inline bool is_completion_signature<execution::SetValue(Args...)> = true;

    template<typename Arg>
    constexpr inline bool is_completion_signature<execution::SetError(Arg)> = true;

    template<>
    constexpr inline bool is_completion_signature<execution::SetStopped()> = true;
}

template<typename T>
concept CompletionSignature = detail::is_completion_signature<T>;
}
