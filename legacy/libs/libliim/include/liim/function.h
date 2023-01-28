#pragma once

#include <liim/pointers.h>
#include <liim/utilities.h>

namespace LIIM {

template<typename>
class Function;

template<typename R, typename... Args>
class Function<R(Args...)> {
public:
    constexpr Function() {};
    constexpr Function(std::nullptr_t) {}

    template<typename Callable,
             typename = typename LIIM::EnableIf<!(IsPointer<Callable>::value &&
                                                  LIIM::IsFunction<typename LIIM::RemovePointer<Callable>::type>::value) &&
                                                LIIM::IsRValueReference<Callable&&>::value>::type>
    constexpr Function(Callable&& callable) : m_closure(make_unique<Closure<Callable>>(move(callable))) {}

    template<typename F, typename = typename LIIM::EnableIf<IsPointer<F>::value &&
                                                            LIIM::IsFunction<typename LIIM::RemovePointer<F>::type>::value>::type>
    constexpr Function(F f) : m_closure(make_unique<Closure<F>>(move(f))) {}

    template<typename Callable,
             typename = typename LIIM::EnableIf<!(IsPointer<Callable>::value &&
                                                  LIIM::IsFunction<typename LIIM::RemovePointer<Callable>::type>::value) &&
                                                LIIM::IsRValueReference<Callable&&>::value>::type>
    constexpr Function& operator=(Callable&& callable) {
        m_closure = make_unique<Closure<Callable>>(move(callable));
        return *this;
    }

    template<typename F, typename = typename LIIM::EnableIf<IsPointer<F>::value &&
                                                            LIIM::IsFunction<typename LIIM::RemovePointer<F>::type>::value>::type>
    constexpr Function& operator=(F function) {
        m_closure = make_unique<Closure<F>>(move(function));
    }

    constexpr Function& operator=(std::nullptr_t) {
        m_closure = nullptr;
        return *this;
    }

    constexpr explicit operator bool() const { return !!m_closure; }
    constexpr bool operator!() const { return !m_closure; }
    constexpr R operator()(Args... args) const { return m_closure->call(forward<Args>(args)...); }

    template<typename = typename LIIM::EnableIf<IsVoid<R>::value>>
    constexpr void safe_call(Args... args) const {
        if (!!m_closure) {
            m_closure->call(forward<Args>(args)...);
        }
    }

    constexpr void swap(Function& other) { swap(this->m_closure, other.m_closure); }

private:
    class ClosureBase {
    public:
        constexpr virtual ~ClosureBase() {}
        constexpr virtual R call(Args...) const = 0;
    };

    template<typename Callable>
    class Closure : public ClosureBase {
    public:
        constexpr explicit Closure(Callable&& c) : m_callable(move(c)) {}
        constexpr virtual ~Closure() {}

        constexpr Closure(const Closure& other) = delete;
        constexpr Closure& operator=(const Closure& other) = delete;

        constexpr virtual R call(Args... args) const final override { return m_callable(forward<Args>(args)...); }

    private:
        Callable m_callable;
    };

    UniquePtr<ClosureBase> m_closure;
};

template<typename R, typename... Args>
Function(R (*)(Args...)) -> Function<R(Args...)>;

template<typename R, typename... Args>
constexpr void swap(Function<R(Args...)>& a, Function<R(Args...)>& b) {
    a.swap(b);
}

}

using LIIM::Function;
