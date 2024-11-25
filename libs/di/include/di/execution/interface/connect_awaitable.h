#pragma once

#include <di/assert/assert_bool.h>
#include <di/execution/concepts/is_awaitable.h>
#include <di/execution/concepts/operation_state.h>
#include <di/execution/concepts/receiver.h>
#include <di/execution/concepts/receiver_of.h>
#include <di/execution/coroutine/with_await_transform.h>
#include <di/execution/meta/await_result.h>
#include <di/execution/receiver/set_error.h>
#include <di/function/invoke.h>
#include <di/platform/prelude.h>
#include <di/util/addressof.h>
#include <di/util/exchange.h>
#include <di/util/immovable.h>
#include <di/util/move.h>
#include <di/util/std_new.h>
#include <di/util/unreachable.h>
#include <di/vocab/error/error.h>
#include <di/vocab/optional/optional_forward_declaration.h>

namespace di::execution {
namespace as_awaitable_ns {
    struct Function;
}

namespace connect_awaitable_ns {
    template<typename Receiver>
    struct OperationStateT {
        struct AllocFailed {};

        struct Type : util::Immovable {
        private:
            struct Promise : WithAwaitTransform<Promise> {
                Receiver& receiver;

                explicit Promise(auto&, Receiver& receiver_) : receiver(receiver_) {}

                auto get_return_object() { return Type { CoroutineHandle<Promise>::from_promise(*this) }; }

                void* operator new(usize size) noexcept { return ::operator new(size, std::nothrow); }

                void operator delete(void* ptr, usize size) noexcept { ::operator delete(ptr, size); }

                static auto get_return_object_on_allocation_failure() { return Type { AllocFailed {} }; }

                template<typename Fn>
                auto yield_value(Fn&& function) noexcept {
                    struct Awaiter {
                        Fn&& function;

                        bool await_ready() noexcept { return false; }
                        void await_suspend(CoroutineHandle<>) { function::invoke(util::forward<Fn>(function)); }
                        void await_resume() { util::unreachable(); }
                    };

                    return Awaiter { util::forward<Fn>(function) };
                }

                CoroutineHandle<> unhandled_error(vocab::Error error) {
                    set_error(util::move(receiver), util::move(error));
                    return noop_coroutine();
                }

                CoroutineHandle<> unhandled_stopped() {
                    set_stopped(util::move(receiver));
                    return noop_coroutine();
                }

                SuspendAlways initial_suspend() noexcept { return {}; }
                SuspendAlways final_suspend() noexcept { util::unreachable(); }
                void return_void() noexcept { util::unreachable(); }
                void unhandled_exception() noexcept { util::unreachable(); }

            private:
                friend decltype(auto) tag_invoke(types::Tag<get_env>, Promise const& self) {
                    return get_env(self.receiver);
                }
            };

        public:
            using promise_type = Promise;

            Type(Type&& other) : m_coroutine(util::exchange(other.m_coroutine, {})) {}

            ~Type() {
                if (m_coroutine) {
                    m_coroutine.destroy();
                }
            }

            void set_receiver(Receiver&& receiver) { m_receiver = util::move(receiver); }

            bool allocation_failed() const { return !m_coroutine; }

        private:
            explicit Type(CoroutineHandle<> coroutine) : m_coroutine(coroutine) {}
            explicit Type(AllocFailed) {}

            friend void tag_invoke(types::Tag<start>, Type& self) {
                if (!self.m_coroutine) {
                    DI_ASSERT(self.m_receiver);
                    execution::set_error(util::move(*self.m_receiver), vocab::Error(BasicError::NotEnoughMemory));
                } else {
                    self.m_coroutine.resume();
                }
            }

            CoroutineHandle<> m_coroutine;
            vocab::Optional<Receiver> m_receiver;
        };
    };

    template<typename Receiver>
    using OperationState = meta::Type<OperationStateT<Receiver>>;

    template<typename Receiver>
    using Promise = OperationState<Receiver>::promise_type;

    template<typename Awaitable, typename Receiver, typename Result = meta::AwaitResult<Awaitable, Promise<Receiver>>>
    struct CompletionSignatures
        : meta::TypeConstant<types::CompletionSignatures<SetValue(Result), SetError(vocab::Error), SetStopped()>> {};

    template<typename Awaitable, typename Receiver, typename Result>
    requires(concepts::LanguageVoid<Result>)
    struct CompletionSignatures<Awaitable, Receiver, Result>
        : meta::TypeConstant<types::CompletionSignatures<SetValue(), SetError(vocab::Error), SetStopped()>> {};

    struct Funciton {
        template<concepts::Receiver Receiver, concepts::IsAwaitable<Promise<Receiver>> Awaitable>
        requires(concepts::ReceiverOf<Receiver, meta::Type<CompletionSignatures<Awaitable, Receiver>>>)
        auto operator()(Awaitable&& awaitable, Receiver receiver) const {
            auto result = impl(util::forward<Awaitable>(awaitable), util::move(receiver));
            if (result.allocation_failed()) {
                result.set_receiver(util::move(receiver));
            }
            return result;
        }

    private:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsubobject-linkage"
        template<typename Awaitable, typename Receiver>
        static OperationState<Receiver> impl(Awaitable awaitable, Receiver receiver) {
            using Result = meta::AwaitResult<Awaitable, Promise<Receiver>>;

            // Connecting any awaitable with a receiver is a matter of returning an operation
            // state, which, once started, enters coroutine context, calls co_await on the awaitable,
            // suspends said coroutine, and finally calls the receiver's completion with the reuslt
            // of co_await.

            // To do so, the OperationState is a coroutine (has a promise type), whose start operation
            // resumes the coroutine. To forcefully suspend the coroutine, we co_yield a lambda expression,
            // which both suspends the coroutine, but also executes the lambda once the suspension occurs.
            if constexpr (concepts::LanguageVoid<Result>) {
                co_await util::move(awaitable);
                co_yield [&] {
                    set_value(util::move(receiver));
                };
            } else {
                auto&& value = co_await util::move(awaitable);
                co_yield [&] {
                    set_value(util::move(receiver), util::forward<decltype(value)>(value));
                };
            }
        }
#pragma GCC diagnostic pop
    };

    constexpr inline auto connect_awaitable = Funciton {};
}
}
