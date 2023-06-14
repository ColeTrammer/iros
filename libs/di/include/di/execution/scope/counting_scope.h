#pragma once

#include <di/concepts/remove_cvref_same_as.h>
#include <di/container/allocator/allocator.h>
#include <di/container/allocator/fallible_allocator.h>
#include <di/container/allocator/infallible_allocator.h>
#include <di/execution/algorithm/just.h>
#include <di/execution/concepts/receiver_of.h>
#include <di/execution/concepts/sender_in.h>
#include <di/execution/interface/connect.h>
#include <di/execution/interface/get_env.h>
#include <di/execution/interface/run.h>
#include <di/execution/interface/start.h>
#include <di/execution/meta/completion_signatures_of.h>
#include <di/execution/meta/connect_result.h>
#include <di/execution/meta/env_of.h>
#include <di/execution/query/get_allocator.h>
#include <di/execution/query/get_completion_signatures.h>
#include <di/execution/query/get_sequence_cardinality.h>
#include <di/execution/query/get_stop_token.h>
#include <di/execution/query/make_env.h>
#include <di/execution/receiver/receiver_adaptor.h>
#include <di/execution/receiver/set_error.h>
#include <di/execution/receiver/set_stopped.h>
#include <di/execution/receiver/set_value.h>
#include <di/execution/scope/scope.h>
#include <di/execution/sequence/sequence_sender.h>
#include <di/execution/types/completion_signuatures.h>
#include <di/execution/types/empty_env.h>
#include <di/function/container/function.h>
#include <di/function/tag_invoke.h>
#include <di/meta/like.h>
#include <di/meta/list/type.h>
#include <di/meta/remove_cvref.h>
#include <di/platform/compiler.h>
#include <di/sync/atomic.h>
#include <di/sync/memory_order.h>
#include <di/sync/stop_token/in_place_stop_source.h>
#include <di/sync/stop_token/in_place_stop_token.h>
#include <di/types/prelude.h>
#include <di/util/addressof.h>
#include <di/util/declval.h>
#include <di/util/defer_construct.h>
#include <di/util/immovable.h>
#include <di/util/reference_wrapper.h>
#include <di/vocab/optional/optional.h>

namespace di::execution {
namespace counting_scope_ns {
    template<typename Alloc>
    struct DataT {
        struct Type {
            [[no_unique_address]] Alloc alloc;
            sync::InPlaceStopSource stop_source;
            sync::Atomic<usize> count { 1 };
            function::Function<void()> did_complete;

            template<typename Env = EmptyEnv>
            auto get_env(Env const& env = {}) const {
                return make_env(env, with(get_allocator, alloc), with(get_stop_token, stop_source.get_stop_token()));
            }

            void start_one() { count.fetch_add(1, sync::MemoryOrder::Relaxed); }

            void complete_one() {
                // The count variable starts at 1, because the cleanup action must start before we call the did_complete
                // function.
                auto old_count = count.fetch_sub(1, sync::MemoryOrder::AcquireRelease);
                if (old_count == 1) {
                    did_complete();
                }
            }
        };
    };

    template<typename Alloc>
    using Data = meta::Type<DataT<Alloc>>;

    template<typename Alloc, typename E = EmptyEnv>
    using Env = decltype(util::declval<Data<Alloc> const&>().get_env(util::declval<E const&>()));

    template<typename Alloc, typename Rec>
    struct NestDataT {
        struct Type {
            Data<Alloc>* data;
            [[no_unique_address]] Rec receiver;
        };
    };

    template<typename Alloc, typename Rec>
    using NestData = meta::Type<NestDataT<Alloc, Rec>>;

    template<typename Alloc, typename Rec>
    struct NestReceiverT {
        struct Type : ReceiverAdaptor<Type> {
        private:
            using Base = ReceiverAdaptor<Type>;
            friend Base;

        public:
            explicit Type(NestData<Alloc, Rec>* data) : m_data(data) {}

            auto base() const& -> Rec const& { return m_data->receiver; }
            auto base() && -> Rec&& { return util::move(m_data->receiver); }

        private:
            template<typename... Vs>
            auto set_value(Vs&&... values) && {
                m_data->data->complete_one();
                execution::set_value(util::move(*this).base(), util::forward<Vs>(values)...);
            }

            template<typename E>
            auto set_error(E&& error) && {
                m_data->data->complete_one();
                execution::set_error(util::move(*this).base(), util::forward<E>(error));
            }

            auto set_stopped() && {
                m_data->data->complete_one();
                execution::set_stopped(util::move(*this).base());
            }

            auto get_env() const& { return m_data->data->get_env(execution::get_env(base())); }

            NestData<Alloc, Rec>* m_data;
        };
    };

    template<typename Alloc, typename Rec>
    using NestReceiver = meta::Type<NestReceiverT<Alloc, Rec>>;

    template<typename Alloc, typename Send, typename Rec>
    struct NestOperationStateT {
        struct Type : util::Immovable {
        public:
            using Op = meta::ConnectResult<Send, NestReceiver<Alloc, Rec>>;

            explicit Type(Data<Alloc>* data, Send&& sender, Rec receiver)
                : m_data(data, util::move(receiver))
                , m_op(connect(util::forward<Send>(sender), NestReceiver<Alloc, Rec>(util::addressof(m_data)))) {}

        private:
            friend void tag_invoke(Tag<start>, Type& self) {
                self.m_data.data->start_one();
                start(self.m_op);
            }

            NestData<Alloc, Rec> m_data;
            DI_IMMOVABLE_NO_UNIQUE_ADDRESS Op m_op;
        };
    };

    template<typename Alloc, typename Send, typename Rec>
    using NestOperationState = meta::Type<NestOperationStateT<Alloc, Send, Rec>>;

    template<typename Alloc, typename Send>
    struct NestSenderT {
        struct Type {
            using is_sender = void;

            [[no_unique_address]] Send sender;
            Data<Alloc>* data;

        private:
            template<concepts::RemoveCVRefSameAs<Type> Self, typename E>
            friend auto tag_invoke(Tag<get_completion_signatures>, Self&&, E&&)
                -> meta::CompletionSignaturesOf<meta::Like<Self, Send>, Env<Alloc, E>>;

            template<concepts::RemoveCVRefSameAs<Type> Self, typename Rec>
            requires(concepts::ReceiverOf<
                     Rec, meta::CompletionSignaturesOf<meta::Like<Self, Send>, Env<Alloc, meta::EnvOf<Rec>>>>)
            friend auto tag_invoke(Tag<connect>, Self&& self, Rec receiver) {
                return NestOperationState<Alloc, meta::Like<Self, Send>, Rec>(
                    self.data, util::forward<Self>(self).sender, util::move(receiver));
            }

            friend auto tag_invoke(Tag<get_env>, Type const& self) { return make_env(self.sender); }
        };
    };

    template<typename Alloc, typename Send>
    using NestSender = meta::Type<NestSenderT<Alloc, meta::RemoveCVRef<Send>>>;

    template<typename Alloc>
    struct CountingScopeT {
        struct Type;

        static auto get_data(Type&) -> Data<Alloc>*;
    };

    template<typename Op, typename Data>
    struct RunReceiverT {
        struct Type {
            using is_receiver = void;

            Op* op;
            Data* data;

            friend void tag_invoke(Tag<set_value>, Type&& self) { self.op->cleanup(); }
            friend void tag_invoke(Tag<set_stopped>, Type&& self) { self.op->cleanup(); }

            friend auto tag_invoke(Tag<get_env>, Type const& self) { return self.data->get_env(); }
        };
    };

    template<typename Op, typename Data>
    using RunReceiver = meta::Type<RunReceiverT<Op, Data>>;

    template<typename Alloc, typename Rec>
    struct RunOperationT {
        struct Type : util::Immovable {
        public:
            using Scope = meta::Type<CountingScopeT<Alloc>>;

            explicit Type(util::ReferenceWrapper<Scope> scope, Rec receiver)
                : m_scope(scope), m_receiver(util::move(receiver)) {}

            void cleanup() {
                auto* data = CountingScopeT<Alloc>::get_data(m_scope);
                data->did_complete = [this] {
                    execution::set_value(util::move(m_receiver));
                };
                data->complete_one();
            }

        private:
            using TokenSender = decltype(just(util::declval<util::ReferenceWrapper<Scope>>()));
            using NextSender = meta::NextSenderOf<Rec, TokenSender>;
            using NextOp = meta::ConnectResult<NextSender, RunReceiver<Type, Data<Alloc>>>;

            friend void tag_invoke(Tag<start>, Type& self) {
                auto& op = self.m_op.emplace(util::DeferConstruct([&] {
                    return connect(set_next(self.m_receiver, just(self.m_scope)),
                                   RunReceiver<Type, Data<Alloc>>(util::addressof(self),
                                                                  CountingScopeT<Alloc>::get_data(self.m_scope)));
                }));
                start(op);
            }

            util::ReferenceWrapper<Scope> m_scope;
            [[no_unique_address]] Rec m_receiver;
            DI_IMMOVABLE_NO_UNIQUE_ADDRESS vocab::Optional<NextOp> m_op;
        };
    };

    template<typename Alloc, typename Rec>
    using RunOperation = meta::Type<RunOperationT<Alloc, Rec>>;

    template<typename Alloc>
    struct RunSequenceT {
        struct Type {
            using Scope = meta::Type<CountingScopeT<Alloc>>;

            using is_sender = SequenceTag;

            util::ReferenceWrapper<Scope> scope;

            using CompletionSignatures =
                di::CompletionSignatures<SetValue(util::ReferenceWrapper<Scope>), SetStopped()>;

            template<typename Rec>
            requires(concepts::SubscriberOf<Rec, CompletionSignatures>)
            friend auto tag_invoke(Tag<subscribe>, Type&& self, Rec receiver) {
                return RunOperation<Alloc, Rec>(self.scope, util::move(receiver));
            }

            friend auto tag_invoke(Tag<get_env>, Type const& self) {
                return make_env(CountingScopeT<Alloc>::get_data(self.scope)->get_env(),
                                with(get_sequence_cardinality, c_<1zu>));
            }
        };
    };

    template<typename Alloc>
    using RunSequence = meta::Type<RunSequenceT<Alloc>>;

    template<typename Alloc>
    struct CountingScopeT<Alloc>::Type : util::Immovable {
    public:
        Data<Alloc>* data() { return util::addressof(m_data); }

    private:
        template<concepts::SenderIn<Env<Alloc>> Send>
        friend auto tag_invoke(Tag<nest>, Type& self, Send&& sender) {
            return NestSender<Alloc, Send>(util::forward<Send>(sender), self.data());
        }

        friend auto tag_invoke(Tag<run>, Type& self) { return RunSequence<Alloc>(self); }

        friend auto tag_invoke(Tag<get_env>, Type const& self) { return self.m_data.get_env(); }

        Data<Alloc> m_data;
    };

    template<typename Alloc>
    auto CountingScopeT<Alloc>::get_data(Type& self) -> Data<Alloc>* {
        return self.data();
    }
}

/// @brief A scope that waits for all spawned senders to complete.
///
/// @tparam Alloc The allocator to use for the scope.
///
/// CountingScope is a scope that waits for all spawned senders to complete, using the async resource mechanism. This
/// means that it can only be accessed using the execution::use_resources function. The provided token allows spawning
/// work, and the async "destructor" will wait for all spawned work to complete.
///
/// This limited API allows the scope to be implemented efficently, with only an atomic counter. This is possible only
/// because the resource API guarantees that we will wait for all spawned work exactly once. More advanced use cases may
/// need to wait multiple times, or store the scope in non-async storage, and would need to use a different scope
/// implementation.
///
/// @see use_resources
/// @see nest
/// @see spawn
/// @see spawn_future
/// @see request_stop
template<concepts::Allocator Alloc = platform::DefaultAllocator>
using CountingScope = meta::Type<counting_scope_ns::CountingScopeT<Alloc>>;
}

namespace di {
using execution::CountingScope;
}
