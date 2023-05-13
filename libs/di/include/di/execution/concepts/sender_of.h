#pragma once

#include <di/execution/concepts/sender.h>
#include <di/execution/meta/gather_signatures.h>
#include <di/execution/meta/matching_sig.h>
#include <di/execution/meta/value_types_of.h>
#include <di/execution/types/no_env.h>
#include <di/meta/list/prelude.h>
#include <di/meta/type_identity.h>

namespace di::concepts {
namespace detail {
    template<typename T>
    struct SenderOfHelper;

    template<typename R, typename... As>
    struct SenderOfHelper<R(As...)> {
        using Tag = R;

        template<typename... Bs>
        using AsSig = R(Bs...);
    };
}

template<typename Send, typename Sig, typename Env = types::NoEnv>
concept SenderOf =
    Sender<Send, Env> &&
    meta::matching_sig<Sig, meta::GatherSignatures<typename detail::SenderOfHelper<Sig>::Tag, Send, Env,
                                                   detail::SenderOfHelper<Sig>::template AsSig, meta::TypeIdentity>>;
}
