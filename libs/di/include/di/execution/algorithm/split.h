#pragma once

#include <di/assert/assert_bool.h>
#include <di/container/allocator/allocate_one.h>
#include <di/container/allocator/allocator.h>
#include <di/container/allocator/deallocate_one.h>
#include <di/container/allocator/fallible_allocator.h>
#include <di/container/allocator/infallible_allocator.h>
#include <di/execution/meta/connect_result.h>
#include <di/execution/meta/make_completion_signatures.h>
#include <di/execution/meta/stop_token_of.h>
#include <di/execution/query/get_stop_token.h>
#include <di/execution/query/make_env.h>
#include <di/execution/receiver/set_error.h>
#include <di/execution/receiver/set_stopped.h>
#include <di/execution/receiver/set_value.h>
#include <di/execution/types/completion_signuatures.h>
#include <di/execution/types/empty_env.h>
#include <di/function/curry_back.h>
#include <di/meta/algorithm.h>
#include <di/platform/compiler.h>
#include <di/sync/atomic.h>
#include <di/sync/memory_order.h>
#include <di/sync/stop_token/in_place_stop_token.h>
#include <di/util/addressof.h>
#include <di/util/immovable.h>
#include <di/vocab/tuple/tuple.h>

namespace di::execution {
namespace split_ns {
    // NOTE: The offical specification uses std::stop_source, which we do not have. An InPlaceStopSource should be fine
    // however, since we manually manage its lifetime.
    using StopSource = sync::InPlaceStopSource;
    using StopToken = sync::InPlaceStopToken;

    template<typename T>
    using DecayedCLValue = meta::Decay<T> const&;

    using Env = MakeEnv<EmptyEnv, With<Tag<get_stop_token>, StopToken>>;

    template<typename... Values>
    using SigSetValue = CompletionSignatures<SetValue(meta::Decay<DecayedCLValue<Values>>...)>;

    template<typename E>
    using SigSetError = CompletionSignatures<SetError(meta::Decay<DecayedCLValue<E>>)>;

    template<typename Send>
    using Sigs =
        meta::MakeCompletionSignatures<Send, Env, CompletionSignatures<SetStopped()>, SigSetValue, SigSetError>;

    struct StopCallbackFunction {
        StopSource& stop_source;

        void operator()() const noexcept { stop_source.request_stop(); }
    };

    struct OperationStateBaseT {
        struct Type : util::Immovable {
            explicit Type(void (*did_complete_)(Type*)) : did_complete(did_complete_) {}

            void (*did_complete)(Type*);
            Type* next { nullptr };
        };
    };

    using OperationStateBase = meta::Type<OperationStateBaseT>;

    template<typename SharedState>
    struct SharedReceiverT {
        struct Type {
            using is_receiver = void;

            SharedState* state;

            template<typename... Args>
            friend void tag_invoke(Tag<set_value>, Type&& self, Args&&... values) {
                self.state->complete_with(set_value, util::forward<Args>(values)...);
            }

            template<typename E>
            friend void tag_invoke(Tag<set_error>, Type&& self, E&& error) {
                self.state->complete_with(set_error, util::forward<E>(error));
            }

            friend void tag_invoke(Tag<set_stopped>, Type&& self) { self.state->complete_with(set_stopped); }

            friend auto tag_invoke(Tag<get_env>, Type const& self) -> Env {
                return make_env(empty_env, with(get_stop_token, self.state->stop_source.get_stop_token()));
            }
        };
    };

    template<typename SharedState>
    using SharedReceiver = meta::Type<SharedReceiverT<SharedState>>;

    template<typename Send, typename SenderAttr, typename Alloc>
    struct SharedStateT {
        struct Type : util::Immovable {
            using Completions = meta::AsList<meta::CompletionSignaturesOf<Send, Env>>;
            using Tags =
                meta::Transform<Completions,
                                meta::Compose<meta::Quote<meta::List>, meta::Quote<meta::LanguageFunctionReturn>>>;
            using DecayedArgs = meta::Transform<Completions, meta::Quote<meta::AsList>>;
            using Storage =
                meta::AsTemplate<vocab::Variant,
                                 meta::Unique<meta::PushFront<
                                     meta::Transform<meta::Zip<Tags, DecayedArgs>,
                                                     meta::Compose<meta::Uncurry<meta::Quote<meta::DecayedTuple>>,
                                                                   meta::Quote<meta::Join>>>,
                                     vocab::Tuple<SetStopped>>>>;

            using Op = meta::ConnectResult<Send, SharedReceiver<Type>>;

            template<typename A>
            explicit Type(Send&& sender, A&& allocator_)
                : sender_attr(get_env(sender))
                , allocator(util::forward<A>(allocator_))
                , operation(connect(util::forward<Send>(sender), SharedReceiver<Type>(this))) {}

            template<typename... Args>
            void complete_with(Args&&... args) {
                storage.template emplace<meta::DecayedTuple<Args...>>(util::forward<Args>(args)...);

                did_complete();
            }

            void did_complete() {
                // Notify all pending operations that the operation has completed. We use ourselves as the sentinel
                // value which indicates that the operation has completed, since we know that we are a valid pointer
                // but are not an operation state. This could instead be done using a bitflag in the pointer, but this
                // is simpler.
                auto* operations = waiting.exchange(static_cast<void*>(this), sync::MemoryOrder::AcquireRelease);

                // Now walk the linked list of pending operations and notify them that the operation has completed.
                // There cannot be races here because we just stole the list of pending operations, so it is not visible
                // to other threads.
                while (operations) {
                    auto* operation = static_cast<OperationStateBase*>(operations);
                    operations = operation->next;
                    operation->did_complete(operation);
                }
            }

            void bump_ref_count() { ref_count.fetch_add(1, sync::MemoryOrder::Relaxed); }
            void drop_ref_count() {
                if (ref_count.fetch_sub(1, sync::MemoryOrder::Release) == 1) {
                    // Destroy the shared state when the reference count reaches 0. Since we store the allocator
                    // ourselves, we must first move it out of the operation state before destroying ourselves.
                    auto allocator = util::move(this->allocator);
                    auto* pointer = this;
                    util::destroy_at(pointer);
                    container::deallocate_one<Type>(allocator, pointer);
                }
            }

            Storage storage;
            StopSource stop_source;
            SenderAttr sender_attr;
            sync::Atomic<usize> ref_count { 1 };
            sync::Atomic<void*> waiting { nullptr };
            [[no_unique_address]] Alloc allocator;
            DI_IMMOVABLE_NO_UNIQUE_ADDRESS Op operation;
        };
    };

    template<typename Send, typename SenderAttr, typename Alloc>
    using SharedState = meta::Type<SharedStateT<Send, meta::Decay<SenderAttr>, meta::Decay<Alloc>>>;

    template<typename Send, typename Rec, typename SenderAttr, typename Alloc>
    struct OperationStateT {
        struct Type : OperationStateBase {
        public:
            using State = SharedState<Send, SenderAttr, Alloc>;

            explicit Type(State* state, Rec receiver)
                : OperationStateBase([](OperationStateBase* void_self) {
                    auto& self = *static_cast<Type*>(void_self);

                    // Reset the stop callback.
                    self.m_stop_callback.reset();

                    // Forward the completion to the receiver.
                    auto& state = *self.m_state;
                    vocab::visit(
                        [&](auto const& tuple) {
                            vocab::apply(
                                [&](auto tag, auto const&... values) {
                                    if constexpr (concepts::SameAs<SetValue, decltype(tag)>) {
                                        set_value(util::move(self.m_receiver), values...);
                                    } else if constexpr (concepts::SameAs<SetError, decltype(tag)>) {
                                        set_error(util::move(self.m_receiver), values...);
                                    } else if constexpr (concepts::SameAs<SetStopped, decltype(tag)>) {
                                        set_stopped(util::move(self.m_receiver));
                                    }
                                },
                                util::forward<decltype(tuple)>(tuple));
                        },
                        util::as_const(state.storage));
                })
                , m_state(state)
                , m_receiver(util::forward<Rec>(receiver)) {}

            ~Type() { m_state->drop_ref_count(); }

        private:
            friend void tag_invoke(Tag<start>, Type& self) {
                auto& state = *self.m_state;

                // Add ourselves to the list of pending operations. The basic idea is to fetch the old head of the list
                // and set out next pointer to it. Then we try to CAS the old head to point to us. If this fails, we try
                // again. Additionally, we stop trying if the old head is the sentinel value, which indicates that the
                // operation has already completed, and just return after forwarding the values. We need to load the old
                // value with acquire semantics to since we write the sentinel value with acquire-release semantics.
                auto* old_head = state.waiting.load(sync::MemoryOrder::Acquire);
                auto* sentinel = static_cast<void*>(self.m_state);
                do {
                    if (old_head == sentinel) {
                        // The operation has already completed, so just return after forwarding the values.
                        self.did_complete(util::addressof(self));
                        return;
                    }

                    self.next = static_cast<OperationStateBase*>(old_head);
                } while (!state.waiting.compare_exchange_weak(old_head, static_cast<void*>(util::addressof(self)),
                                                              sync::MemoryOrder::Release, sync::MemoryOrder::Acquire));

                // Emplace the stop callback.
                self.m_stop_callback.emplace(execution::get_stop_token(execution::get_env(self.m_receiver)),
                                             StopCallbackFunction { state.stop_source });

                if (old_head == nullptr) {
                    // We were the first operation to be added to the list, so we must start the operation.
                    // If stop was requested, complete all operations with set stopped.
                    if (state.stop_source.stop_requested()) {
                        state.did_complete();
                    } else {
                        // Start the operation.
                        start(state.operation);
                    }
                }
            }

            State* m_state;
            Rec m_receiver;
            vocab::Optional<typename meta::StopTokenOf<meta::EnvOf<Rec>>::template CallbackType<StopCallbackFunction>>
                m_stop_callback;
        };
    };

    template<typename Send, typename Rec, typename SenderAttr, typename Alloc>
    using OperationState = meta::Type<OperationStateT<Send, Rec, SenderAttr, Alloc>>;

    template<typename Send, typename Alloc>
    struct SenderT {
        struct Type {
        public:
            using is_sender = void;

            using CompletionSignatures = Sigs<Send>;

            using State = SharedState<Send, meta::EnvOf<Send>, Alloc>;

            explicit Type(State* state) : m_state(state) { DI_ASSERT(state); }

            Type(Type const& other) : m_state(other.m_state) {
                if (m_state) {
                    m_state->bump_ref_count();
                }
            }
            Type(Type&& other) : m_state(util::exchange(other.m_state, nullptr)) {}

            ~Type() {
                auto state = util::exchange(m_state, nullptr);
                if (state) {
                    state->drop_ref_count();
                }
            }

            // NOLINTNEXTLINE(bugprone-unhandled-self-assignment,cert-oop54-cpp)
            Type& operator=(Type const& other) {
                if (m_state != other.m_state) {
                    auto state = util::exchange(m_state, other.m_state);
                    if (state) {
                        state->drop_ref_count();
                    }
                    if (m_state) {
                        m_state->bump_ref_count();
                    }
                }
                return *this;
            }
            Type& operator=(Type&& other) {
                this->m_state = util::exchange(other.m_state, nullptr);
                return *this;
            }

        private:
            template<concepts::ReceiverOf<CompletionSignatures> Rec>
            friend auto tag_invoke(Tag<connect>, Type const& self, Rec receiver) {
                DI_ASSERT(self.m_state);
                self.m_state->bump_ref_count();
                return OperationState<Send, Rec, meta::EnvOf<Send>, Alloc>(self.m_state, util::move(receiver));
            }

            template<concepts::ReceiverOf<CompletionSignatures> Rec>
            friend auto tag_invoke(Tag<connect>, Type&& self, Rec receiver) {
                DI_ASSERT(self.m_state);
                return OperationState<Send, Rec, meta::EnvOf<Send>, Alloc>(util::exchange(self.m_state, nullptr),
                                                                           util::move(receiver));
            }

            friend auto tag_invoke(Tag<get_env>, Type const& self) -> meta::Decay<meta::EnvOf<Send>> const& {
                DI_ASSERT(self.m_state);
                return self.m_state->sender_attr;
            }

            State* m_state { nullptr };
        };
    };

    template<typename Send, typename Alloc>
    using Sender = meta::Type<SenderT<Send, meta::Decay<Alloc>>>;

    struct Function {
        template<concepts::SenderIn<Env> Send, concepts::Allocator Alloc = platform::DefaultAllocator>
        requires(concepts::DecayConstructible<meta::EnvOf<Send>>)
        auto operator()(Send&& sender, Alloc&& allocator = {}) const {
            if constexpr (requires {
                              tag_invoke(*this, get_completion_scheduler<SetValue>(get_env(sender)),
                                         util::forward<Send>(sender), util::forward<Alloc>(allocator));
                          }) {
                return tag_invoke(*this, get_completion_scheduler<SetValue>(get_env(sender)),
                                  util::forward<Send>(sender), util::forward<Alloc>(allocator));
            } else if constexpr (requires {
                                     tag_invoke(*this, util::forward<Send>(sender), util::forward<Alloc>(allocator));
                                 }) {
                return tag_invoke(*this, util::forward<Send>(sender), util::forward<Alloc>(allocator));
            } else {
                using State = SharedState<Send, meta::EnvOf<Send>, Alloc>;
                return vocab::as_fallible(container::allocate_one<State>(allocator)) % [&](State* state) {
                    util::construct_at(state, util::forward<Send>(sender), util::forward<Alloc>(allocator));
                    return Sender<Send, Alloc>(state);
                } | vocab::try_infallible;
            }
        }
    };
}

/// @brief Split a sender into a sender which sends the same value to multiple receivers.
///
/// @param sender The sender to split.
/// @param allocator The allocator to use for the shared state (optional).
///
/// @returns A sender which sends the same value to multiple receivers, or an error if allocation failed.
///
/// This function enables a sender to be split into multiple receivers. The returned sender will send the same value to
/// all receivers. This requires an atomic queue of waiting receivers, which is stored in a heap-allocated shared state.
/// The underlying sender is only started when the first operation is started.
///
/// @note The returned sender does not copy-out the values, but passes them by const reference.
constexpr inline auto split = function::curry_back(split_ns::Function {}, c_<2zu>);
}
