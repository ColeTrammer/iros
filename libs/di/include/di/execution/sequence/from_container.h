#pragma once

#include <di/container/concepts/input_container.h>
#include <di/container/concepts/view.h>
#include <di/container/interface/begin.h>
#include <di/container/interface/end.h>
#include <di/container/iterator/iterator_move.h>
#include <di/container/meta/container_iterator.h>
#include <di/container/meta/container_rvalue.h>
#include <di/execution/concepts/receiver.h>
#include <di/execution/concepts/receiver_of.h>
#include <di/execution/interface/connect.h>
#include <di/execution/interface/get_env.h>
#include <di/execution/interface/start.h>
#include <di/execution/meta/connect_result.h>
#include <di/execution/meta/env_of.h>
#include <di/execution/meta/stop_token_of.h>
#include <di/execution/query/get_completion_signatures.h>
#include <di/execution/query/get_stop_token.h>
#include <di/execution/query/is_always_lockstep_sequence.h>
#include <di/execution/query/make_env.h>
#include <di/execution/receiver/set_stopped.h>
#include <di/execution/receiver/set_value.h>
#include <di/execution/sequence/sequence_sender.h>
#include <di/execution/types/completion_signuatures.h>
#include <di/execution/types/empty_env.h>
#include <di/function/pipeable.h>
#include <di/function/tag_invoke.h>
#include <di/meta/constexpr.h>
#include <di/meta/core.h>
#include <di/meta/util.h>
#include <di/platform/compiler.h>
#include <di/sync/concepts/stoppable_token.h>
#include <di/sync/concepts/unstoppable_token.h>
#include <di/util/addressof.h>
#include <di/util/defer_construct.h>
#include <di/util/immovable.h>
#include <di/util/move.h>
#include <di/vocab/optional/optional_forward_declaration.h>

namespace di::execution {
namespace from_container_ns {
    template<typename Con, typename Rec>
    struct DataT {
        struct Type {
            using Env = meta::EnvOf<Rec>;
            using StopToken = meta::StopTokenOf<Env>;
            using Iterator = meta::ContainerIterator<Con>;

            explicit Type(Con container_, Rec receiver_)
                : container(util::move(container_))
                , receiver(util::move(receiver_))
                , iterator(container::begin(container))
                , stop_token(get_stop_token(get_env(receiver))) {}

            [[no_unique_address]] Con container;
            [[no_unique_address]] Rec receiver;
            [[no_unique_address]] Iterator iterator;
            [[no_unique_address]] StopToken stop_token;
        };
    };

    template<concepts::InputContainer Con, concepts::Receiver Rec>
    using Data = meta::Type<DataT<Con, Rec>>;

    template<typename Con, typename Rec>
    struct OperationStateT {
        struct Type;
    };

    template<concepts::InputContainer Con, concepts::Receiver Rec>
    using OperationState = meta::Type<OperationStateT<Con, Rec>>;

    template<typename Con, typename Rec>
    struct NextReceiverT {
        struct Type {
            using is_receiver = void;

            OperationState<Con, Rec>* op;

            friend void tag_invoke(types::Tag<set_value>, Type&& self) { self.op->report_value(); }
            friend void tag_invoke(types::Tag<set_stopped>, Type&& self) { self.op->report_stopped(); }
        };
    };

    template<concepts::InputContainer Con, concepts::Receiver Rec>
    using NextReceiver = meta::Type<NextReceiverT<Con, Rec>>;

    template<typename Con, typename Rec, typename R>
    struct NextOperationStateT {
        struct Type {
        public:
            explicit Type(Data<Con, Rec>* data, R receiver) : m_data(data), m_receiver(util::move(receiver)) {}

        private:
            friend void tag_invoke(types::Tag<start>, Type& self) {
                auto&& value = container::iterator_move(self.m_data->iterator);
                ++self.m_data->iterator;
                set_value(util::move(self.m_receiver), util::forward<decltype(value)>(value));
            }

            Data<Con, Rec>* m_data;
            [[no_unique_address]] R m_receiver;
        };
    };

    template<concepts::InputContainer Con, concepts::Receiver Rec, concepts::Receiver R>
    using NextOperationState = meta::Type<NextOperationStateT<Con, Rec, R>>;

    template<typename Con, typename Rec>
    struct NextSenderT {
        struct Type {
            using is_sender = void;

            using CompletionSignatures = types::CompletionSignatures<SetValue(meta::ContainerRValue<Con>)>;

            Data<Con, Rec>* data;

            template<concepts::ReceiverOf<CompletionSignatures> R>
            friend auto tag_invoke(types::Tag<connect>, Type&& self, R receiver) {
                return NextOperationState<Con, Rec, R>(self.data, util::move(receiver));
            }
        };
    };

    template<concepts::InputContainer Con, concepts::Receiver Rec>
    using NextSender = meta::Type<NextSenderT<Con, Rec>>;

    template<typename Con, typename Rec>
    struct OperationStateT<Con, Rec>::Type : util::Immovable {
    public:
        using Env = meta::EnvOf<Rec>;
        using StopToken = meta::StopTokenOf<Env>;
        using Send = meta::NextSenderOf<Rec, NextSender<Con, Rec>>;
        using Receiver = NextReceiver<Con, Rec>;
        using Op = meta::ConnectResult<Send, Receiver>;

        explicit Type(Con container, Rec receiver) : m_data(util::move(container), util::move(receiver)) {}

        void report_value() {
            if (m_data.iterator == container::end(m_data.container)) {
                return set_value(util::move(m_data.receiver));
            }

            m_operation.emplace(util::DeferConstruct([&] {
                return connect(set_next(m_data.receiver, NextSender<Con, Rec>(util::addressof(m_data))),
                               Receiver(this));
            }));
            start(*m_operation);
        }

        void report_stopped() { set_value(util::move(m_data.receiver)); }

    private:
        friend void tag_invoke(types::Tag<start>, Type& self) { self.report_value(); }

        [[no_unique_address]] Data<Con, Rec> m_data;
        DI_IMMOVABLE_NO_UNIQUE_ADDRESS vocab::Optional<Op> m_operation;
    };

    template<typename Con, typename Env>
    using Signatures = types::CompletionSignatures<SetValue(meta::ContainerRValue<Con>)>;

    template<typename Con>
    struct SequenceT {
        struct Type {
            using is_sender = SequenceTag;

            [[no_unique_address]] Con container;

            template<concepts::RemoveCVRefSameAs<Type> Self, typename Rec>
            requires(concepts::DecayConstructible<meta::Like<Self, Con>> &&
                     concepts::SubscriberOf<Rec, Signatures<Con, meta::EnvOf<Rec>>>)
            friend auto tag_invoke(types::Tag<subscribe>, Self&& self, Rec receiver) {
                return OperationState<Con, Rec>(util::forward<Self>(self).container, util::move(receiver));
            }

            template<concepts::RemoveCVRefSameAs<Type> Self, typename Env>
            requires(concepts::DecayConstructible<meta::Like<Self, Con>>)
            friend auto tag_invoke(types::Tag<get_completion_signatures>, Self&&, Env&&) -> Signatures<Con, Env>;

            friend auto tag_invoke(Tag<get_env>, Type const&) {
                return make_env(empty_env, with(is_always_lockstep_sequence, c_<true>));
            }
        };
    };

    template<concepts::InputContainer Con>
    requires(concepts::MovableValue<Con>)
    using Sequence = meta::Type<SequenceT<meta::Decay<Con>>>;

    struct ValidLifetimeTag {};

    struct Function : function::pipeline::EnablePipeline {
        template<concepts::InputContainer Con>
        requires(concepts::MovableValue<Con> && !concepts::View<Con>)
        auto operator()(Con&& container) const {
            return Sequence<Con>(util::forward<Con>(container));
        }

        template<concepts::InputContainer Con>
        requires(concepts::MovableValue<Con>)
        auto operator()(ValidLifetimeTag, Con&& container) const {
            return Sequence<Con>(util::forward<Con>(container));
        }
    };
}

/// @brief Tag type to indicate that the container lifetime is valid for the async sequence.
///
/// @see from_container
constexpr inline auto valid_lifetime = from_container_ns::ValidLifetimeTag {};

/// @brief Creates a sequence sender from a container.
///
/// @param container The container.
///
/// @return A sequence sender which emits all values from the container.
///
/// @note The container is decay copied into sequence. When using a view instead, pass the execution::valid_lifetime tag
/// as the first argument, which allows the caller to assert that the view references data which will be valid for the
/// lifetime of the sequence. This should only be used for data with static lifetime (e.g. string literals), or when
/// referencing data which is stored in an operation state, when using a algorithm like let_value().
///
/// @note If the receiver's stop token is not an unstoppable token, the sequence will stop sending values if a stop is
/// requested.
constexpr inline auto from_container = from_container_ns::Function {};
}
