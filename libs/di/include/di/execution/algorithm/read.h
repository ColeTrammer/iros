#pragma once

#include <di/concepts/decay_constructible.h>
#include <di/execution/concepts/operation_state.h>
#include <di/execution/concepts/receiver.h>
#include <di/execution/concepts/sender.h>
#include <di/execution/concepts/sender_of.h>
#include <di/execution/interface/connect.h>
#include <di/execution/interface/get_env.h>
#include <di/execution/query/get_allocator.h>
#include <di/execution/query/get_delegatee_scheduler.h>
#include <di/execution/query/get_scheduler.h>
#include <di/execution/query/get_stop_token.h>
#include <di/function/invoke.h>

namespace di::execution {
namespace read_ns {
    template<typename Tag>
    struct SenderT {
        struct Type {
            using is_sender = void;

            template<typename Receiver>
            struct OperationStateT {
                struct Type {
                    Receiver receiver;

                private:
                    friend void tag_invoke(types::Tag<execution::start>, Type& self) {
                        set_value(util::move(self.receiver), auto(Tag {}(get_env(self.receiver))));
                    }
                };
            };

            template<typename Receiver>
            using OperationState = meta::Type<OperationStateT<Receiver>>;

            template<concepts::Receiver Receiver>
            requires(concepts::DecayConstructible<Receiver>)
            friend auto tag_invoke(types::Tag<connect>, Type, Receiver&& receiver) {
                return OperationState<meta::Decay<Receiver>> { util::forward<Receiver>(receiver) };
            }

            template<typename Env>
            requires(concepts::Invocable<Tag, Env>)
            friend auto tag_invoke(types::Tag<get_completion_signatures>, Type, Env&&)
                -> types::CompletionSignatures<SetValue(meta::InvokeResult<Tag, Env>)>;
        };
    };

    template<typename Tag>
    using Sender = meta::Type<SenderT<Tag>>;

    struct Function {
        template<typename Tag>
        constexpr auto operator()(Tag) const {
            return Sender<Tag> {};
        }
    };
}

constexpr inline auto read = read_ns::Function {};

namespace detail {
    constexpr auto GetSchedulerFunction::operator()() const {
        return read(get_scheduler);
    }

    constexpr auto GetDelegateeSchedulerFunction::operator()() const {
        return read(get_delegatee_scheduler);
    }

    constexpr auto GetStopTokenFunction::operator()() const {
        return read(get_stop_token);
    }

    constexpr auto GetAllocatorFunction::operator()() const {
        return read(get_allocator);
    }

    constexpr auto GetEnvFunction::operator()() const {
        return read(function::identity);
    }
}
}
