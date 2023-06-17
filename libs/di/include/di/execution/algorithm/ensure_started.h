#pragma once

#include <di/assert/assert_bool.h>
#include <di/concepts/decay_constructible.h>
#include <di/container/allocator/allocator.h>
#include <di/container/allocator/fallible_allocator.h>
#include <di/container/allocator/infallible_allocator.h>
#include <di/execution/concepts/receiver_of.h>
#include <di/execution/concepts/sender_in.h>
#include <di/execution/interface/connect.h>
#include <di/execution/interface/get_env.h>
#include <di/execution/interface/start.h>
#include <di/execution/meta/completion_signatures_of.h>
#include <di/execution/meta/connect_result.h>
#include <di/execution/meta/decayed_tuple.h>
#include <di/execution/meta/env_of.h>
#include <di/execution/meta/make_completion_signatures.h>
#include <di/execution/meta/stop_token_of.h>
#include <di/execution/query/get_stop_token.h>
#include <di/execution/query/make_env.h>
#include <di/execution/receiver/set_error.h>
#include <di/execution/receiver/set_stopped.h>
#include <di/execution/receiver/set_value.h>
#include <di/execution/types/empty_env.h>
#include <di/function/container/prelude.h>
#include <di/meta/decay.h>
#include <di/meta/list/as_list.h>
#include <di/meta/list/push_front.h>
#include <di/meta/list/type.h>
#include <di/meta/remove_cvref.h>
#include <di/platform/compiler.h>
#include <di/sync/atomic.h>
#include <di/sync/memory_order.h>
#include <di/sync/stop_token/in_place_stop_source.h>
#include <di/sync/stop_token/in_place_stop_token.h>
#include <di/types/in_place.h>
#include <di/types/integers.h>
#include <di/types/prelude.h>
#include <di/util/addressof.h>
#include <di/util/exchange.h>
#include <di/util/immovable.h>
#include <di/vocab/optional/optional_forward_declaration.h>
#include <di/vocab/tuple/tuple.h>
#include <dius/platform.h>

namespace di::execution {
namespace ensure_started_ns {
    // NOTE: The offical specification uses std::stop_source, which we do not have. An InPlaceStopSource should be fine
    // however, since we manually manage its lifetime.
    using StopSource = sync::InPlaceStopSource;
    using StopToken = sync::InPlaceStopToken;

    template<typename T>
    using DecayedRValue = meta::Decay<T>&&;

    using Env = MakeEnv<EmptyEnv, With<Tag<get_stop_token>, StopToken>>;

    template<typename... Values>
    using SigSetValue = CompletionSignatures<SetValue(meta::Decay<Values>&&...)>;

    template<typename E>
    using SigSetError = CompletionSignatures<SetError(meta::Decay<E>&&)>;

    template<typename Seq>
    using Sigs = meta::MakeCompletionSignatures<Seq, Env, CompletionSignatures<>, SigSetValue, SigSetError>;

    struct StopCallbackFunction {
        StopSource& stop_source;

        void operator()() const noexcept { stop_source.request_stop(); }
    };

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
            using Storage = meta::AsTemplate<
                vocab::Variant,
                meta::PushFront<
                    meta::Unique<meta::Transform<
                        meta::Zip<Tags, DecayedArgs>,
                        meta::Compose<meta::Uncurry<meta::Quote<meta::DecayedTuple>>, meta::Quote<meta::Join>>>>,
                    vocab::Tuple<Void>>>;

            using Op = meta::ConnectResult<Send, SharedReceiver<Type>>;

            template<typename A>
            explicit Type(Send&& sender, A&& allocator_)
                : did_complete([this] {
                    // Destroy the shared state when the sender completes. Since we store the allocator
                    // ourselves, we must first move it out of the operation state before destroying ourselves.
                    auto allocator = util::move(this->allocator);
                    auto* pointer = this;
                    util::destroy_at(pointer);
                    container::deallocate_one<Type>(allocator, pointer);
                })
                , sender_attr(get_env(sender))
                , operation(connect(util::forward<Send>(sender), SharedReceiver<Type>(this)))
                , allocator(util::forward<A>(allocator_)) {
                start(operation);
            }

            template<typename... Args>
            void complete_with(Args&&... args) {
                storage.template emplace<meta::DecayedTuple<Args...>>(util::forward<Args>(args)...);
                finish_one();
            }

            void finish_one(bool request_stop = false) {
                auto old = ref_count.fetch_sub(1, sync::MemoryOrder::AcquireRelease);
                if (old == 1) {
                    did_complete();
                } else if (request_stop) {
                    stop_source.request_stop();
                }
            }

            Function<void()> did_complete;
            Storage storage;
            StopSource stop_source;
            SenderAttr sender_attr;
            sync::Atomic<usize> ref_count { 2 };
            [[no_unique_address]] Alloc allocator;
            DI_IMMOVABLE_NO_UNIQUE_ADDRESS Op operation;
        };
    };

    template<typename Send, typename SenderAttr, typename Alloc>
    using SharedState = meta::Type<SharedStateT<Send, meta::Decay<SenderAttr>, meta::Decay<Alloc>>>;

    template<typename Send, typename Rec, typename SenderAttr, typename Alloc>
    struct OperationStateT {
        struct Type : util::Immovable {
        public:
            using State = SharedState<Send, SenderAttr, Alloc>;

            explicit Type(State* state, Rec receiver) : m_state(state), m_receiver(util::forward<Rec>(receiver)) {}

            ~Type() {
                auto started = util::exchange(m_started, true);
                if (!started) {
                    m_state->finish_one(true);
                }
            }

        private:
            friend void tag_invoke(Tag<start>, Type& self) {
                self.m_started = true;

                auto completion_callback = [&self] {
                    // Reset the stop callback.
                    self.m_stop_callback.reset();

                    // Forward the completion to the receiver.
                    auto& state = *self.m_state;
                    vocab::visit(
                        [&](auto&& tuple) {
                            vocab::apply(
                                [&](auto tag, auto&&... values) {
                                    if constexpr (concepts::SameAs<SetValue, decltype(tag)>) {
                                        set_value(util::move(self.m_receiver),
                                                  util::forward<decltype(values)>(values)...);
                                    } else if constexpr (concepts::SameAs<SetError, decltype(tag)>) {
                                        set_error(util::move(self.m_receiver),
                                                  util::forward<decltype(values)>(values)...);
                                    } else if constexpr (concepts::SameAs<SetStopped, decltype(tag)>) {
                                        set_stopped(util::move(self.m_receiver));
                                    }
                                },
                                util::forward<decltype(tuple)>(tuple));
                        },
                        util::move(state.storage));

                    // Destroy the shared state when the sender completes. Since we store the allocator
                    // ourselves, we must first move it out of the operation state before destroying ourselves.
                    auto allocator = util::move(state.allocator);
                    auto* pointer = util::addressof(state);
                    util::destroy_at(pointer);
                    container::deallocate_one<State>(allocator, pointer);
                };

                // If the original sender has finished, report the completions:
                auto& state = *self.m_state;
                if (state.ref_count.load(sync::MemoryOrder::Relaxed) == 1) {
                    state.did_complete = completion_callback;
                    return state.finish_one();
                }

                // Emplace the stop callback.
                self.m_stop_callback.emplace(execution::get_stop_token(execution::get_env(self.m_receiver)),
                                             StopCallbackFunction { state.stop_source });

                // If stop was requested, just complete with set_stopped().
                if (state.stop_source.stop_requested()) {
                    set_stopped(util::move(self.m_receiver));
                    return state.finish_one();
                }

                // Register the completion callback.
                state.did_complete = completion_callback;
                state.finish_one();
            }

            State* m_state;
            Rec m_receiver;
            vocab::Optional<typename meta::StopTokenOf<meta::EnvOf<Rec>>::template CallbackType<StopCallbackFunction>>
                m_stop_callback;
            bool m_started { false };
        };
    };

    template<typename Send, typename Rec, typename SenderAttr, typename Alloc>
    using OperationState = meta::Type<OperationStateT<Send, Rec, meta::Decay<SenderAttr>, Alloc>>;

    template<typename Send, typename Alloc>
    struct SenderT {
        struct Type {
        public:
            using is_sender = void;

            using CompletionSignatures = Sigs<Send>;

            using State = SharedState<Send, meta::EnvOf<Send>, Alloc>;

            explicit Type(State* state) : m_state(state) { DI_ASSERT(state); }

            Type(Type&& other) : m_state(util::exchange(other.m_state, nullptr)) {}

            ~Type() {
                auto state = util::exchange(m_state, nullptr);
                if (state) {
                    state->finish_one(true);
                }
            }

            Type& operator=(Type&& other) {
                this->m_state = util::exchange(other.m_state, nullptr);
                return *this;
            }

        private:
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

/// @brief Eagerly start a sender.
///
/// @param sender The sender to start.
/// @param allocator The allocator to use for the shared state (optional).
///
/// @return A sender that represents the original sender, but with the guarantee that it has started.
///
/// This function is used to eagerly start a sender, while still being able to retrieve its result later. It is
/// implemented using a shared state, which is allocated using the provided allocator. If no allocator is provided, the
/// default allocator is used.
///
/// If the returned sender is destroyed before it has been started, the eagerly started sender will be signalled by
/// means of a stop token, possibly causing it to be cancelled. When the eagerly started sender completes, its values
/// are decayed copied into the shared state, which is then used to complete the returned sender.
///
/// This function is an unstructured version of execution::spawn_future(), which is used to eagerly start a sender in a
/// provided async scope. This ensures that the eagerly started sender completes before the async scope is destroyed.
///
/// @see spawn_future
constexpr inline auto ensure_started = ensure_started_ns::Function {};
}
