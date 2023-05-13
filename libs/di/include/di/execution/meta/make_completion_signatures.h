#pragma once

#include <di/execution/concepts/sender.h>
#include <di/execution/concepts/valid_completion_signatures.h>
#include <di/execution/meta/error_types_of.h>
#include <di/execution/meta/sends_stopped.h>
#include <di/execution/meta/value_types_of.h>

namespace di::meta {
namespace detail {
    template<typename... Values>
    using DefaultSetValue = types::CompletionSignatures<execution::SetValue(Values...)>;

    template<typename Error>
    using DefaultSetError = types::CompletionSignatures<execution::SetError(Error)>;

    template<typename A, typename B, typename C, typename D>
    struct MakeCompletionSignaturesHelper : TypeConstant<types::DependentCompletionSignatures<types::NoEnv>> {};

    template<
        concepts::InstanceOf<types::CompletionSignatures> As, concepts::InstanceOf<types::CompletionSignatures>... Bs,
        concepts::InstanceOf<types::CompletionSignatures>... Cs, concepts::InstanceOf<types::CompletionSignatures> Ds>
    struct MakeCompletionSignaturesHelper<As, meta::List<Bs...>, meta::List<Cs...>, Ds>
        : TypeConstant<meta::AsTemplate<types::CompletionSignatures,
                                        meta::Unique<meta::Concat<meta::AsList<As>, meta::AsList<Bs>...,
                                                                  meta::AsList<Cs>..., meta::AsList<Ds>>>>> {};
}

template<typename Send, typename Env = types::NoEnv,
         concepts::ValidCompletionSignatures<Env> ExtraSigs = types::CompletionSignatures<>,
         template<typename...> typename SetValue = detail::DefaultSetValue,
         template<typename> typename SetError = detail::DefaultSetError,
         concepts::ValidCompletionSignatures<Env> SetStopped = types::CompletionSignatures<execution::SetStopped()>>
requires(concepts::Sender<Send, Env>)
using MakeCompletionSignatures = Type<detail::MakeCompletionSignaturesHelper<
    ExtraSigs, meta::ValueTypesOf<Send, Env, SetValue, meta::List>,
    meta::Transform<meta::ErrorTypesOf<Send, Env, meta::List>, meta::Quote<SetError>>,
    meta::Conditional<meta::sends_stopped<Send, Env>, SetStopped, types::CompletionSignatures<>>>>;
}
