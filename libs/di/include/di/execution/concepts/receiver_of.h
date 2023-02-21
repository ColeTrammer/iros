#pragma once

#include <di/execution/concepts/receiver.h>
#include <di/execution/types/completion_signuatures.h>
#include <di/function/tag_invoke.h>

namespace di::concepts {
namespace detail {
    template<typename Signature, typename T>
    concept ValidCompletionFor = requires(Signature* signature) {
                                     []<typename Ret, typename... Args>(Ret(*)(Args...))
                                     requires concepts::TagInvocable<Ret, meta::RemoveCVRef<T>, Args...>
                                     {}
                                     (signature);
                                 };
}

template<class T, class Completions>
concept ReceiverOf =
    Receiver<T> &&
    requires(Completions* completions) {
        []<detail::ValidCompletionFor<T>... Signatures>(types::CompletionSignatures<Signatures...>*) {}(completions);
    };
}
