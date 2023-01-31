#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/iterator/default_sentinel.h>
#include <di/container/iterator/iterator_base.h>
#include <di/container/meta/prelude.h>
#include <di/container/types/prelude.h>
#include <di/container/view/view_interface.h>
#include <di/util/coroutine.h>
#include <di/util/exchange.h>
#include <di/util/unreachable.h>

namespace di::function {
namespace detail {
    template<typename Ref, typename Value>
    using GeneratorValue = meta::Conditional<concepts::LanguageVoid<Value>, meta::RemoveCVRef<Ref>, Value>;

    template<typename Ref, typename Value>
    using GeneratorReference = meta::Conditional<concepts::LanguageVoid<Value>, Ref&&, Ref>;

    template<typename Ref>
    using GeneratorYield = meta::Conditional<concepts::Reference<Ref>, Ref, Ref const&>;

    template<concepts::Reference Yield>
    class GeneratorPromiseBase {
    private:
        struct FinalAwaiter {
            bool await_ready() noexcept { return false; }
            CoroutineHandle<> await_suspend(CoroutineHandle<>) noexcept { return noop_coroutine(); }
            void await_resume() noexcept {}
        };

    public:
        auto initial_suspend() noexcept { return SuspendAlways {}; }
        auto final_suspend() noexcept { return FinalAwaiter {}; }

        auto yield_value(Yield value) noexcept {
            m_pointer = util::addressof(value);
            return SuspendAlways {};
        }

        void await_transform() = delete;

        void return_void() noexcept {}

        void unhandled_exception() { util::unreachable(); }

    private:
        template<typename, typename>
        friend class GeneratorIterator;

        meta::AddPointer<Yield> m_pointer { nullptr };
    };

    template<typename Ref, typename Value>
    class GeneratorIterator
        : public container::IteratorBase<GeneratorIterator<Ref, Value>, InputIteratorTag, GeneratorValue<Ref, Value>,
                                         ssize_t> {
    private:
        using Handle = CoroutineHandle<GeneratorPromiseBase<GeneratorYield<Ref>>>;

    public:
        GeneratorIterator(InPlace, Handle coroutine) : m_coroutine(coroutine) {}

        GeneratorIterator(GeneratorIterator&& other) : m_coroutine(util::exchange(other.m_coroutine, {})) {}

        GeneratorIterator& operator=(GeneratorIterator&& other) {
            m_coroutine = util::exchange(other.m_coroutine);
            return *this;
        }

        Ref operator*() const {
            DI_ASSERT(!m_coroutine.done());
            return static_cast<Ref>(*m_coroutine.promise().m_pointer);
        }

        void advance_one() {
            DI_ASSERT(!m_coroutine.done());
            m_coroutine.resume();
        }

    private:
        friend bool operator==(GeneratorIterator const& a, container::DefaultSentinel) { return a.m_coroutine.done(); }

        Handle m_coroutine;
    };
}

template<typename Ref, typename Value = void>
class Generator : public container::ViewInterface<Generator<Ref, Value>> {
    using BasePromiseType = detail::GeneratorPromiseBase<detail::GeneratorYield<Ref>>;

    struct PromiseType : BasePromiseType {
        Generator get_return_object() noexcept {
            return Generator { in_place, CoroutineHandle<PromiseType>::from_promise(*this) };
        }
    };

public:
    using promise_type = PromiseType;

    Generator(Generator&& other) : m_coroutine(util::exchange(other.m_coroutine, {})) {}

    ~Generator() {
        if (m_coroutine) {
            m_coroutine.destroy();
        }
    }

    Generator& operator=(Generator other) {
        util::swap(this->m_coroutine, other.m_coroutine);
        return *this;
    }

    auto begin() {
        DI_ASSERT(m_coroutine);
        m_coroutine.resume();
        return detail::GeneratorIterator<Ref, Value> { in_place, CoroutineHandle<BasePromiseType>::from_address(
                                                                     m_coroutine.address()) };
    }

    auto end() const { return container::default_sentinel; }

private:
    explicit Generator(InPlace, CoroutineHandle<PromiseType> handle) : m_coroutine(handle) {}

    CoroutineHandle<PromiseType> m_coroutine {};
};
}