#pragma once

#include <di/any/concepts/any_storable_infallibly.h>
#include <di/any/concepts/any_storage.h>
#include <di/any/container/prelude.h>
#include <di/any/storage/inline_storage.h>
#include <di/any/types/method.h>
#include <di/any/types/this.h>
#include <di/any/vtable/maybe_inline_vtable.h>
#include <di/concepts/constructible_from.h>
#include <di/concepts/derived_from.h>
#include <di/execution/any/any_env.h>
#include <di/execution/concepts/completion_signature.h>
#include <di/execution/concepts/receiver_of.h>
#include <di/execution/concepts/valid_completion_signatures.h>
#include <di/execution/receiver/set_error.h>
#include <di/meta/algorithm.h>
#include <di/meta/core.h>
#include <di/meta/language_function_return.h>
#include <di/meta/remove_cvref.h>
#include <di/vocab/error/error.h>

namespace di::execution {
namespace detail {
    template<concepts::CompletionSignature Sig>
    using MethodForSig =
        types::Method<meta::LanguageFunctionReturn<Sig>,
                      meta::AsLanguageFunction<void, meta::PushFront<meta::AsList<Sig>, types::This&&>>>;

    template<concepts::ValidCompletionSignatures Sigs>
    using AnySigs =
        meta::AsTemplate<types::CompletionSignatures,
                         meta::PushFront<meta::PushFront<meta::AsList<Sigs>, SetError(vocab::Error)>, SetStopped()>>;

    template<concepts::ValidCompletionSignatures Sigs, typename Env>
    using AnyReceiverMethods =
        InterfaceWithEnv<meta::Transform<meta::Unique<meta::AsList<AnySigs<Sigs>>>, meta::Quote<MethodForSig>>, Env>;
}

template<typename Sigs, typename Env, typename Storage, typename VTablePolicy>
struct AnyReceiverT {
    class Type : public Any<detail::AnyReceiverMethods<Sigs, Env>, Storage, VTablePolicy> {
    private:
        using Base = Any<detail::AnyReceiverMethods<Sigs, Env>, Storage, VTablePolicy>;

    public:
        using is_receiver = void;

        Type(Type const&) = delete;
        Type& operator=(Type const&) = delete;

        Type(Type&&) = default;
        Type& operator=(Type&&) = default;

        template<typename R, typename T = meta::RemoveCVRef<R>>
        requires(!concepts::DerivedFrom<T, Type> && concepts::ReceiverOf<T, detail::AnySigs<Sigs>> &&
                 concepts::ConstructibleFrom<T, R> && concepts::AnyStorableInfallibly<T, typename Base::AnyStorage>)
        Type(R&& receiver) : Base(util::forward<R>(receiver)) {}
    };
};

template<concepts::ValidCompletionSignatures Sigs, typename Env = void,
         concepts::AnyStorage Storage = any::InlineStorage<2 * sizeof(void*), alignof(void*)>,
         typename VTablePolicy = any::MaybeInlineVTable<3>>
using AnyReceiver = meta::Type<AnyReceiverT<Sigs, Env, Storage, VTablePolicy>>;
}

namespace di {
using execution::AnyReceiver;
}
