#pragma once

#include <di/assert/assert_bool.h>
#include <di/concepts/language_void.h>
#include <di/concepts/reference.h>
#include <di/execution/algorithm/just.h>
#include <di/execution/coroutine/with_awaitable_senders.h>
#include <di/execution/sequence/async_range.h>
#include <di/execution/sequence/sequence_sender.h>
#include <di/function/tag_invoke.h>
#include <di/meta/add_pointer.h>
#include <di/meta/core.h>
#include <di/meta/remove_cvref.h>
#include <di/util/coroutine.h>
#include <di/util/exchange.h>
#include <di/util/immovable.h>
#include <di/vocab/error/error.h>
#include <di/vocab/optional/nullopt.h>
#include <di/vocab/optional/optional_forward_declaration.h>

namespace di::execution {
namespace async_generator_ns {
    template<typename Ref, typename Value>
    struct AsyncGeneratorT;

    template<typename Ref, typename Value>
    using GeneratorValue = meta::Conditional<concepts::LanguageVoid<Value>, meta::RemoveCVRef<Ref>, Value>;

    template<typename Ref, typename Value>
    using GeneratorReference = meta::Conditional<concepts::LanguageVoid<Value>, Ref&&, Ref>;

    template<typename Ref>
    using GeneratorYield = meta::Conditional<concepts::Reference<Ref>, Ref, Ref const&>;

    struct AllocFailed {};

    template<typename Self, typename Ref, typename Value>
    struct PromiseBaseT {
        struct Type : WithAwaitableSenders<Self> {
            using PromiseBase = Type;
            using Yield = GeneratorYield<Ref>;

        public:
            Type() = default;

            void* operator new(usize size) noexcept { return ::operator new(size, std::nothrow); }
            void operator delete(void* ptr, usize size) noexcept { ::operator delete(ptr, size); }

            SuspendAlways initial_suspend() noexcept { return {}; }
            auto final_suspend() noexcept { return FinalAwaiter {}; }

            struct YieldAwaiter {
                bool await_ready() noexcept { return false; }

                template<typename Promise>
                CoroutineHandle<> await_suspend(CoroutineHandle<Promise> coroutine) noexcept {
                    PromiseBase& current = coroutine.promise();
                    return current.continuation() ? current.continuation() : noop_coroutine();
                }

                void await_resume() noexcept {}
            };

            auto yield_value(Yield value) noexcept {
                m_pointer = util::addressof(value);
                return YieldAwaiter {};
            }

            void return_value(types::Void) noexcept {}

            template<typename E>
            requires(concepts::ConstructibleFrom<vocab::Error, E>)
            void return_value(vocab::Unexpected<E>&& error) noexcept {
                m_error = util::move(error).error();
            }

            void return_value(Stopped) noexcept { m_error = BasicError::OperationCanceled; }

            void unhandled_exception() { util::unreachable(); }

        private:
            template<typename, typename>
            friend struct AsyncGeneratorT;

            struct FinalAwaiter {
                bool await_ready() noexcept { return false; }

                template<typename Promise>
                CoroutineHandle<> await_suspend(CoroutineHandle<Promise> coroutine) noexcept {
                    PromiseBase& current = coroutine.promise();

                    auto was_error = current.m_error.has_value();
                    if (was_error) {
                        if (current.m_error == BasicError::OperationCanceled) {
                            return current.unhandled_stopped();
                        }
                        return current.unhandled_error(util::move(current.m_error).value());
                    }

                    return current.continuation() ? current.continuation() : noop_coroutine();
                }

                void await_resume() noexcept {}
            };

            struct AsyncRange : util::Immovable {
                CoroutineHandle<PromiseBase> coroutine;

                explicit AsyncRange(CoroutineHandle<PromiseBase> coroutine_) noexcept : coroutine(coroutine_) {}

                ~AsyncRange() noexcept {
                    if (coroutine) {
                        coroutine.destroy();
                    }
                }

                struct NextAwaiter {
                    CoroutineHandle<PromiseBase> coroutine;

                    bool await_ready() noexcept { return false; }

                    template<typename OtherPromise>
                    CoroutineHandle<> await_suspend(CoroutineHandle<OtherPromise> continuation) noexcept {
                        DI_ASSERT(coroutine);

                        coroutine.promise().set_continuation(continuation);
                        return coroutine;
                    }

                    vocab::Optional<Value> await_resume() noexcept {
                        DI_ASSERT(coroutine);

                        auto& promise = coroutine.promise();
                        DI_ASSERT(!promise.m_error.has_value());

                        if (coroutine.done()) {
                            return vocab::nullopt;
                        }

                        return *promise.m_pointer;
                    }
                };

                friend auto tag_invoke(types::Tag<next>, AsyncRange& self) { return NextAwaiter { self.coroutine }; }
            };

            struct Awaiter {
                CoroutineHandle<PromiseBase> coroutine;

                bool await_ready() noexcept {
                    // No need to suspend if we have a coroutine.
                    return !!coroutine;
                }

                template<typename OtherPromise>
                CoroutineHandle<> await_suspend(CoroutineHandle<OtherPromise> continuation) noexcept {
                    DI_ASSERT(!coroutine);
                    return continuation.promise().unhandled_error(vocab::Error(BasicError::NotEnoughMemory));
                }

                auto await_resume() noexcept {
                    DI_ASSERT(coroutine);
                    return AsyncRange(coroutine);
                }
            };

            meta::AddPointer<Yield> m_pointer { nullptr };
            vocab::Optional<vocab::Error> m_error {};
        };
    };

    template<typename Self, typename Ref, typename Value>
    using PromiseBase = meta::Type<PromiseBaseT<Self, Ref, Value>>;

    template<typename Ref, typename Value>
    struct [[nodiscard]] AsyncGeneratorT {
        struct Type {
        private:
            struct Promise;

            using PromiseBase = async_generator_ns::PromiseBase<Promise, Ref, GeneratorValue<Ref, Value>>;

            struct Promise : PromiseBase {
                Type get_return_object() noexcept { return Type { CoroutineHandle<Promise>::from_promise(*this) }; }
                static Type get_return_object_on_allocation_failure() noexcept { return Type { AllocFailed {} }; }
            };

            using Handle = CoroutineHandle<Promise>;
            using ParentHandle = CoroutineHandle<PromiseBase>;
            using Awaiter = PromiseBase::Awaiter;

        public:
            using promise_type = Promise;

            Type(Type&& other) : m_handle(util::exchange(other.m_handle, {})) {}

            ~Type() {
                if (m_handle) {
                    m_handle.destroy();
                }
            }

            Awaiter operator co_await() && {
                auto handle = util::exchange(m_handle, {});
                if (!handle) {
                    return Awaiter { nullptr };
                }
                auto& promise = static_cast<PromiseBase&>(handle.promise());
                return Awaiter { ParentHandle::from_promise(promise) };
            }

        private:
            explicit Type(Handle handle) : m_handle(handle) {}
            explicit Type(AllocFailed) {}

            Handle m_handle {};
        };
    };
}

template<typename Ref, typename Value = void>
using AsyncGenerator = meta::Type<async_generator_ns::AsyncGeneratorT<Ref, Value>>;
}

namespace di {
using execution::AsyncGenerator;
}
