#pragma once

#include <di/assert/assert_bool.h>
#include <di/function/invoke.h>
#include <di/meta/algorithm.h>
#include <di/meta/constexpr.h>
#include <di/meta/language.h>
#include <di/meta/util.h>
#include <di/types/prelude.h>
#include <di/util/addressof.h>

namespace di::function {
namespace function_ref_ns {
    template<typename F, typename T>
    struct SignatureAfterBindFrontHelper;

    template<typename T, typename R, typename U, typename... Args>
    struct SignatureAfterBindFrontHelper<R (*)(U, Args...), T> : meta::TypeConstant<R(Args...)> {};

    template<typename T, typename M, typename G>
    requires(concepts::Object<M>)
    struct SignatureAfterBindFrontHelper<M G::*, T> : meta::TypeConstant<meta::InvokeResult<M G::*, T>> {};

    template<typename T, typename M, typename G>
    requires(concepts::LanguageFunction<M>)
    struct SignatureAfterBindFrontHelper<M G::*, T> : meta::TypeConstant<meta::RemoveFunctionQualifiers<M>> {};

    template<typename F, typename T>
    using SignatureAfterBindFront = meta::Type<SignatureAfterBindFrontHelper<F, T>>;

    template<typename Function>
    struct SignatureInfo;

    template<typename R, typename... Args>
    struct SignatureInfo<R(Args...)> {
        using Type = R(Args...);

        template<typename... Fs>
        constexpr static bool is_invocable = concepts::InvocableR<R, Fs..., Args...>;

        constexpr static bool is_noexcept = false;

        template<typename T>
        using Qualified = T;
    };

    template<typename R, typename... Args>
    struct SignatureInfo<R(Args...) const> {
        using Type = R(Args...);

        template<typename... Fs>
        constexpr static bool is_invocable = concepts::InvocableR<R, Fs..., Args...>;

        constexpr static bool is_noexcept = false;

        template<typename T>
        using Qualified = T const;
    };

    template<typename R, typename... Args>
    struct SignatureInfo<R(Args...) noexcept> {
        using Type = R(Args...);

        template<typename... Fs>
        constexpr static bool is_invocable = concepts::InvocableR<R, Fs..., Args...>;

        constexpr static bool is_noexcept = true;

        template<typename T>
        using Qualified = T;
    };

    template<typename R, typename... Args>
    struct SignatureInfo<R(Args...) const noexcept> {
        using Type = R(Args...);

        template<typename... Fs>
        constexpr static bool is_invocable = concepts::InvocableR<R, Fs..., Args...>;

        constexpr static bool is_noexcept = true;

        template<typename T>
        using Qualified = T const;
    };

    union ErasedStorage {
        void* pointer { nullptr };
        void const* const_pointer;
        void (*function_pointer)();

        constexpr ErasedStorage() = default;

        template<concepts::Object T>
        constexpr explicit ErasedStorage(T* pointer_) : pointer(pointer_) {}

        template<concepts::Object T>
        constexpr explicit ErasedStorage(T const* const_pointer_) : const_pointer(const_pointer_) {}

        template<concepts::LanguageFunction T>
        explicit ErasedStorage(T* function_pointer_)
            : function_pointer(reinterpret_cast<void (*)()>(function_pointer_)) {}
    };

    template<typename T>
    constexpr auto down_cast(ErasedStorage storage) {
        if constexpr (concepts::Const<T>) {
            return static_cast<T*>(storage.const_pointer);
        } else if constexpr (concepts::Object<T>) {
            return static_cast<T*>(storage.pointer);
        } else {
            return reinterpret_cast<T*>(storage.function_pointer);
        }
    }

    template<typename Sig, typename = meta::Type<SignatureInfo<Sig>>>
    class FunctionRef;

    template<typename Sig, typename R, typename... Args>
    class FunctionRef<Sig, R(Args...)> {
    private:
        using Info = SignatureInfo<Sig>;

        template<typename... Fs>
        constexpr static bool is_invocable = Info::template is_invocable<Fs...>;

        constexpr static bool is_noexcept = Info::is_noexcept;

        template<typename T>
        using CVQualified = Info::template Qualified<T>;

        // For function ref, every function is lvalue qualified.
        template<typename T>
        using Qualified = CVQualified<T>&;

        using ErasedFunctionPointer = R (*)(ErasedStorage, Args&&...) noexcept(is_noexcept);

    public:
        template<concepts::LanguageFunction F>
        requires(is_invocable<F>)
        constexpr FunctionRef(F* function)
            : m_storage(function), m_impl([](ErasedStorage storage, Args&&... args) noexcept(is_noexcept) -> R {
                return function::invoke_r<R>(down_cast<F>(storage), util::forward<Args>(args)...);
            }) {
            DI_ASSERT(function != nullptr);
        }

        template<typename F, typename T = meta::RemoveReference<F>>
        requires(!concepts::RemoveCVRefSameAs<F, FunctionRef> && !concepts::MemberPointer<T> &&
                 is_invocable<Qualified<F>>)
        // NOLINTNEXTLINE(bugprone-forwarding-reference-overload)
        constexpr FunctionRef(F&& function)
            : m_storage(util::addressof(function))
            , m_impl([](ErasedStorage storage, Args&&... args) noexcept(is_noexcept) -> R {
                // Ensure we are invoking the object with the correct const and lvalue qualifications.
                Qualified<T> object_reference = *down_cast<T>(storage);
                return function::invoke_r<R>(object_reference, util::forward<Args>(args)...);
            }) {}

        template<auto f, typename F = decltype(f)>
        requires(is_invocable<F>)
        constexpr FunctionRef(Constexpr<f>)
            : m_impl([](ErasedStorage, Args&&... args) noexcept(is_noexcept) -> R {
                return function::invoke_r<R>(f, util::forward<Args>(args)...);
            }) {
            if constexpr (concepts::Pointer<F> || concepts::MemberPointer<F>) {
#ifndef DI_SANITIZER
                static_assert(f != nullptr, "FunctionRef Constexpr<> constructors cannot be passed a nullptr.");
#endif
            }
        }

        template<auto f, typename U, typename F = decltype(f), typename T = meta::RemoveReference<U>>
        requires(!concepts::RValueReference<U &&> && is_invocable<F, Qualified<T>>)
        constexpr FunctionRef(Constexpr<f>, U&& object)
            : m_storage(util::addressof(object))
            , m_impl([](ErasedStorage storage, Args&&... args) noexcept(is_noexcept) -> R {
                // Ensure we are invoking the object with the correct const and lvalue qualifications.
                Qualified<T> object_reference = *down_cast<T>(storage);
                return function::invoke_r<R>(f, object_reference, util::forward<Args>(args)...);
            }) {
            if constexpr (concepts::Pointer<F> || concepts::MemberPointer<F>) {
#ifndef DI_SANITIZER
                static_assert(f != nullptr, "FunctionRef Constexpr<> constructors cannot be passed a nullptr.");
#endif
            }
        }

        template<auto f, typename T, typename F = decltype(f)>
        requires(is_invocable<F, CVQualified<T>*>)
        constexpr FunctionRef(Constexpr<f>, CVQualified<T>* object)
            : m_storage(object), m_impl([](ErasedStorage storage, Args&&... args) noexcept(is_noexcept) -> R {
                return function::invoke_r<R>(f, down_cast<CVQualified<T>>(storage), util::forward<Args>(args)...);
            }) {
            if constexpr (concepts::Pointer<F> || concepts::MemberPointer<F>) {
#ifndef DI_SANITIZER
                static_assert(f != nullptr, "FunctionRef Constexpr<> constructors cannot be passed a nullptr.");
#endif
            }
            DI_ASSERT(object != nullptr);
        }

        template<typename T>
        requires(!concepts::SameAs<T, FunctionRef> && !concepts::Pointer<T> && is_invocable<Qualified<T>>)
        auto operator=(T) -> FunctionRef& = delete;

        constexpr auto operator()(Args... args) const noexcept(is_noexcept) -> R {
            return m_impl(m_storage, util::forward<Args>(args)...);
        }

    private:
        ErasedStorage m_storage {};
        ErasedFunctionPointer m_impl { nullptr };
    };

    template<concepts::LanguageFunction F>
    FunctionRef(F*) -> FunctionRef<F>;

    template<auto f, typename F = meta::RemovePointer<decltype(f)>>
    requires(concepts::LanguageFunction<F>)
    FunctionRef(Constexpr<f>) -> FunctionRef<F>;

    template<auto f, typename T, typename F = decltype(f)>
    FunctionRef(Constexpr<f>, T&&) -> FunctionRef<SignatureAfterBindFront<F, T>>;
}

using function_ref_ns::FunctionRef;
}

namespace di {
using function::FunctionRef;
}
