#pragma once

#include <di/execution/concepts/sender.h>
#include <di/execution/meta/completion_signatures_of.h>
#include <di/execution/types/completion_signuatures.h>
#include <di/meta/false_type.h>
#include <di/meta/true_type.h>

namespace di::concepts {
namespace detail {
    template<typename Completions>
    struct SendsStopHelper : meta::FalseType {};

    template<>
    struct SendsStopHelper<types::CompletionSignatures<>> : meta::FalseType {};

    template<typename T, typename... Rest>
    struct SendsStopHelper<types::CompletionSignatures<T, Rest...>>
        : SendsStopHelper<types::CompletionSignatures<Rest...>> {};

    template<typename... Rest>
    struct SendsStopHelper<types::CompletionSignatures<execution::SetStopped(), Rest...>> : meta::TrueType {};
}

template<typename Send, typename Env = types::NoEnv>
concept SendsStopped = Sender<Send, Env> && detail::SendsStopHelper<meta::CompletionSignaturesOf<Send, Env>>::value;
}
