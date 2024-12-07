#pragma once

#include "di/execution/algorithm/just.h"
#include "di/execution/algorithm/let.h"
#include "di/execution/algorithm/read.h"
#include "di/execution/algorithm/then.h"
#include "di/execution/concepts/prelude.h"
#include "di/execution/interface/get_env.h"
#include "di/execution/meta/prelude.h"
#include "di/execution/receiver/prelude.h"
#include "di/execution/types/prelude.h"

namespace di::execution {
namespace stopped_as_optional_ns {
    struct Function : function::pipeline::EnablePipeline {
        template<concepts::Sender Send>
        requires(concepts::DecayConstructible<Send>)
        auto operator()(Send&& sender) const -> concepts::Sender auto {
            return execution::let_value(get_env(), [sender = util::forward<Send>(sender)]<typename E>(E const&)
                                        requires(concepts::SingleSender<Send, E>)
                                        {
                                            using Opt = Optional<meta::Decay<meta::SingleSenderValueType<Send, E>>>;
                                            return execution::let_stopped(
                                                execution::then(util::move(sender),
                                                                []<typename T>(T&& value) {
                                                                    return Opt(util::forward<T>(value));
                                                                }),
                                                [] {
                                                    return execution::just(Opt());
                                                });
                                        });
        }
    };
}

constexpr inline auto stopped_as_optional = stopped_as_optional_ns::Function {};
}
