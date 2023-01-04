#pragma once

#include <di/execution/concepts/sender.h>
#include <di/execution/concepts/valid_completion_signatures.h>

namespace di::meta {
namespace detail {
    template<typename... Values>
    using DefaultSetValue = types::CompletionSignatures<execution::SetValue(Values...)>;

    template<typename Error>
    using DefaultSetError = types::CompletionSignatures<execution::SetError(Error)>;

    template<typename A, typename B>
    struct MakeCompletionSignaturesHelper : TypeConstant<types::DependentCompletionSignatures<types::NoEnv>> {};

    template<typename... As, typename... Bs>
    struct MakeCompletionSignaturesHelper<types::CompletionSignatures<As...>, types::CompletionSignatures<Bs...>>
        : TypeConstant<meta::AsTemplate<types::CompletionSignatures, meta::Unique<meta::List<As..., Bs...>>>> {};
}

template<typename Send, typename Env = types::NoEnv, concepts::ValidCompletionSignatures<Env> ExtraSigs = types::CompletionSignatures<>,
         template<typename...> typename SetValue = detail::DefaultSetValue, template<typename> typename SetError = detail::DefaultSetError,
         concepts::ValidCompletionSignatures<Env> SetStopped = types::CompletionSignatures<execution::SetStopped()>>
requires(concepts::Sender<Send, Env>)
using MakeCompletionSignatures = Type<detail::MakeCompletionSignaturesHelper<ExtraSigs, meta::CompletionSignaturesOf<Send, Env>>>;
}