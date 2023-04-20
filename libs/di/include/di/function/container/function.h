#pragma once

#include <di/assert/assert_bool.h>
#include <di/concepts/object.h>
#include <di/function/invoke.h>
#include <di/function/monad/monad_try.h>
#include <di/meta/list/prelude.h>
#include <di/meta/maybe_const.h>
#include <di/meta/remove_function_qualifiers.h>
#include <di/types/prelude.h>
#include <di/util/addressof.h>
#include <di/util/construct_at.h>
#include <di/util/destroy_at.h>
#include <di/util/exchange.h>
#include <di/util/initializer_list.h>
#include <di/util/std_new.h>
#include <di/vocab/array/prelude.h>
#include <di/vocab/error/prelude.h>

namespace di::function {
namespace function_ns {
    template<typename Function>
    struct SignatureInfo;

    template<typename R, typename... Args>
    struct SignatureInfo<R(Args...)> {
        using Type = R(Args...);

        template<typename... Fs>
        constexpr static bool is_invocable = concepts::InvocableR<R, Fs..., Args...>;

        constexpr static bool is_noexcept = false;

        template<typename T>
        using CVQualified = T;

        template<typename T>
        using RefQualified = T;
    };

    template<typename R, typename... Args>
    struct SignatureInfo<R(Args...) const> {
        using Type = R(Args...);

        template<typename... Fs>
        constexpr static bool is_invocable = concepts::InvocableR<R, Fs..., Args...>;

        constexpr static bool is_noexcept = false;

        template<typename T>
        using CVQualified = T const;

        template<typename T>
        using RefQualified = T;
    };

    template<typename R, typename... Args>
    struct SignatureInfo<R(Args...)&> {
        using Type = R(Args...);

        template<typename... Fs>
        constexpr static bool is_invocable = concepts::InvocableR<R, Fs..., Args...>;

        constexpr static bool is_noexcept = false;

        template<typename T>
        using CVQualified = T;

        template<typename T>
        using RefQualified = T&;
    };

    template<typename R, typename... Args>
    struct SignatureInfo<R(Args...) const&> {
        using Type = R(Args...);

        template<typename... Fs>
        constexpr static bool is_invocable = concepts::InvocableR<R, Fs..., Args...>;

        constexpr static bool is_noexcept = false;

        template<typename T>
        using CVQualified = T const;

        template<typename T>
        using RefQualified = T&;
    };

    template<typename R, typename... Args>
    struct SignatureInfo<R(Args...) &&> {
        using Type = R(Args...);

        template<typename... Fs>
        constexpr static bool is_invocable = concepts::InvocableR<R, Fs..., Args...>;

        constexpr static bool is_noexcept = false;

        template<typename T>
        using CVQualified = T;

        template<typename T>
        using RefQualified = T&&;
    };

    template<typename R, typename... Args>
    struct SignatureInfo<R(Args...) const&&> {
        using Type = R(Args...);

        template<typename... Fs>
        constexpr static bool is_invocable = concepts::InvocableR<R, Fs..., Args...>;

        constexpr static bool is_noexcept = false;

        template<typename T>
        using CVQualified = T const;

        template<typename T>
        using RefQualified = T&&;
    };

    template<typename R, typename... Args>
    struct SignatureInfo<R(Args...) noexcept> {
        using Type = R(Args...);

        template<typename... Fs>
        constexpr static bool is_invocable = concepts::InvocableR<R, Fs..., Args...>;

        constexpr static bool is_noexcept = true;

        template<typename T>
        using CVQualified = T;

        template<typename T>
        using RefQualified = T;
    };

    template<typename R, typename... Args>
    struct SignatureInfo<R(Args...) const noexcept> {
        using Type = R(Args...);

        template<typename... Fs>
        constexpr static bool is_invocable = concepts::InvocableR<R, Fs..., Args...>;

        constexpr static bool is_noexcept = true;

        template<typename T>
        using CVQualified = T const;

        template<typename T>
        using RefQualified = T;
    };

    template<typename R, typename... Args>
    struct SignatureInfo<R(Args...) & noexcept> {
        using Type = R(Args...);

        template<typename... Fs>
        constexpr static bool is_invocable = concepts::InvocableR<R, Fs..., Args...>;

        constexpr static bool is_noexcept = true;

        template<typename T>
        using CVQualified = T;

        template<typename T>
        using RefQualified = T&;
    };

    template<typename R, typename... Args>
    struct SignatureInfo<R(Args...) const & noexcept> {
        using Type = R(Args...);

        template<typename... Fs>
        constexpr static bool is_invocable = concepts::InvocableR<R, Fs..., Args...>;

        constexpr static bool is_noexcept = true;

        template<typename T>
        using CVQualified = T const;

        template<typename T>
        using RefQualified = T&;
    };

    template<typename R, typename... Args>
    struct SignatureInfo<R(Args...) && noexcept> {
        using Type = R(Args...);

        template<typename... Fs>
        constexpr static bool is_invocable = concepts::InvocableR<R, Fs..., Args...>;

        constexpr static bool is_noexcept = true;

        template<typename T>
        using CVQualified = T;

        template<typename T>
        using RefQualified = T&&;
    };

    template<typename R, typename... Args>
    struct SignatureInfo<R(Args...) const && noexcept> {
        using Type = R(Args...);

        template<typename... Fs>
        constexpr static bool is_invocable = concepts::InvocableR<R, Fs..., Args...>;

        constexpr static bool is_noexcept = true;

        template<typename T>
        using CVQualified = T const;

        template<typename T>
        using RefQualified = T&&;
    };

    struct ErasedStorage {
        union {
            void* pointer;
            alignas(2 * sizeof(void*)) di::Array<di::Byte, 2 * sizeof(void*)> byte_storage;
        };

        void* address() { return util::addressof(byte_storage[0]); }
        void const* address() const { return util::addressof(byte_storage[0]); }
    };

    template<typename F>
    concept StoredInline =
        sizeof(F) <= sizeof(ErasedStorage) && alignof(F) <= alignof(ErasedStorage) && concepts::MoveConstructible<F>;

    struct ErasedObject {
    private:
        using ThunkFunction = void (*)(ErasedStorage*, ErasedStorage*);

        template<typename T>
        static void concrete_thunk(ErasedStorage* a, ErasedStorage* b) {
            if constexpr (StoredInline<T>) {
                if (b) {
                    // Move from b into a.
                    auto* b_value = static_cast<T*>(b->address());
                    util::construct_at(static_cast<T*>(a->address()), util::move(*b_value));
                    util::destroy_at(b_value);
                } else {
                    // Just destroy a.
                    util::destroy_at(static_cast<T*>(a->address()));
                }
            } else {
                if (b) {
                    // Move from b into a.
                    a->pointer = util::exchange(b->pointer, nullptr);
                } else {
                    // Just destroy a.
                    delete static_cast<T*>(a->pointer);
                }
            }
        }

    public:
        ErasedObject() {}

        ErasedObject(ErasedObject const&) = delete;
        ErasedObject(ErasedObject&& other) {
            if ((m_thunk = util::exchange(other.m_thunk, nullptr))) {
                m_thunk(util::addressof(m_storage), util::addressof(other.m_storage));
            }
        }

        ~ErasedObject() { reset(); }

        ErasedObject& operator=(ErasedObject const&) = delete;
        ErasedObject& operator=(ErasedObject&& other) {
            reset();
            if ((m_thunk = util::exchange(other.m_thunk, nullptr))) {
                m_thunk(util::addressof(m_storage), util::addressof(other.m_storage));
            }
            return *this;
        }

        bool empty() const { return m_thunk != nullptr; }
        void reset() {
            if (auto* old_thunk = util::exchange(m_thunk, nullptr)) {
                old_thunk(util::addressof(m_storage), nullptr);
            }
        }

        template<typename T>
        T* down_cast() {
            if constexpr (StoredInline<T>) {
                return static_cast<T*>(m_storage.address());
            } else {
                return static_cast<T*>(m_storage.pointer);
            }
        }

        template<typename T>
        T* down_cast() const {
            if constexpr (StoredInline<T>) {
                return static_cast<T*>(m_storage.address());
            } else {
                return static_cast<T*>(m_storage.pointer);
            }
        }

        template<typename T, typename... Args>
        requires(StoredInline<T>)
        void init_inline(Args&&... args) {
            m_thunk = &concrete_thunk<T>;
            util::construct_at(this->down_cast<T>(), util::forward<Args>(args)...);
        }

        template<typename T, typename... Args>
        requires(!StoredInline<T>)
        Result<void> init_out_of_line_fallible(Args&&... args) {
            auto* pointer = new (std::nothrow) T(util::forward<Args>(args)...);
            if (!pointer) {
                return Unexpected(BasicError::FailedAllocation);
            }
            m_thunk = &concrete_thunk<T>;
            m_storage.pointer = pointer;
            return {};
        }

        template<typename T, typename... Args>
        requires(!StoredInline<T>)
        void init_out_of_line_infallible(Args&&... args) {
            auto* pointer = new (std::nothrow) T(util::forward<Args>(args)...);
            DI_ASSERT(pointer);
            m_thunk = &concrete_thunk<T>;
            m_storage.pointer = pointer;
        }

    private:
        ThunkFunction m_thunk { nullptr };
        ErasedStorage m_storage;
    };

    template<typename T>
    struct MakeFunction;

    template<typename T>
    struct TryMakeFunction;

    template<typename Sig, typename = meta::Type<SignatureInfo<Sig>>>
    class Function;

    template<typename Sig, typename R, typename... Args>
    class Function<Sig, R(Args...)> {
    private:
        template<typename>
        friend struct MakeFunction;

        template<typename>
        friend struct TryMakeFunction;

        using Info = SignatureInfo<Sig>;

        template<typename... Fs>
        constexpr static bool is_invocable = Info::template is_invocable<Fs...>;

        constexpr static bool is_noexcept = Info::is_noexcept;

        template<typename T>
        using CVQualified = Info::template CVQualified<T>;

        template<typename T>
        using RefQualified = Info::template RefQualified<T>;

        template<typename T>
        using Qualified = RefQualified<CVQualified<T>>;

        constexpr static bool is_const = concepts::SameAs<CVQualified<int>, int const>;
        constexpr static bool is_lvalue = concepts::SameAs<RefQualified<int>, int&>;
        constexpr static bool is_rvalue = concepts::SameAs<RefQualified<int>, int&&>;

        template<typename T>
        using InvQualifed = meta::Conditional<is_lvalue || is_rvalue, Qualified<T>, CVQualified<T>&>;

        template<typename VT>
        constexpr static bool is_callable_from = is_invocable<Qualified<VT>> && is_invocable<InvQualifed<VT>>;

        template<auto f, typename VT>
        constexpr static bool is_callable_as_if_from = is_invocable<decltype(f), InvQualifed<VT>>;

        using ErasedFunctionPointer = R (*)(meta::MaybeConst<is_const, ErasedObject>*, Args&&...) noexcept(is_noexcept);

        template<typename T>
        static R concrete_impl(meta::MaybeConst<is_const, ErasedObject>* object, Args&&... args) noexcept(is_noexcept) {
            using CV = CVQualified<T>;
            using Inv = InvQualifed<T>;
            return function::invoke_r<R>(util::forward<Inv>(*object->template down_cast<CV>()),
                                         util::forward<Args>(args)...);
        }

        template<auto f>
        static R concrete_impl_for_nontype(meta::MaybeConst<is_const, ErasedObject>*,
                                           Args&&... args) noexcept(is_noexcept) {
            return function::invoke_r<R>(f, util::forward<Args>(args)...);
        }

        template<auto f, typename T>
        static R concrete_impl_for_bound_nontype(meta::MaybeConst<is_const, ErasedObject>* object, Args&&... args) {
            using CV = CVQualified<T>;
            using Inv = InvQualifed<T>;
            return function::invoke_r<R>(f, util::forward<Inv>(*object->template down_cast<CV>()),
                                         util::forward<Args>(args)...);
        }

    public:
        Function() = default;
        Function(nullptr_t) : Function() {}

        Function(Function const&) = delete;
        Function(Function&& other)
            : m_object(util::move(other.m_object)), m_impl(util::exchange(other.m_impl, nullptr)) {}

        template<auto f>
        requires(is_invocable<decltype(f)>)
        Function(Nontype<f>) {
            m_impl = &concrete_impl_for_nontype<f>;
        }

        template<typename F, typename VT = meta::Decay<F>>
        requires(!concepts::SameAs<meta::RemoveCVRef<F>, Function> &&
                 !concepts::InstanceOf<meta::RemoveCVRef<F>, InPlaceType> && is_callable_from<VT> && StoredInline<VT>)
        Function(F&& object) {
            if constexpr (concepts::MemberPointer<VT> ||
                          concepts::InstanceOf<meta::RemoveCVRef<F>, function_ns::Function>) {
                if (object == nullptr) {
                    return;
                }
            }
            m_object.init_inline<VT>(util::forward<F>(object));
            m_impl = &concrete_impl<VT>;
        }

        template<auto f, typename T, typename VT = meta::Decay<T>>
        requires(concepts::ConstructibleFrom<VT, T> && is_callable_as_if_from<f, VT> && StoredInline<VT>)
        Function(Nontype<f>, T&& object) {
            m_object.init_inline<VT>(util::forward<T>(object));
            m_impl = &concrete_impl_for_bound_nontype<f, VT>;
        }

        template<typename T, typename... Ts, typename VT = meta::Decay<T>>
        requires(concepts::ConstructibleFrom<T, Ts...> && is_callable_from<VT> && StoredInline<VT>)
        explicit Function(InPlaceType<T>, Ts&&... args) {
            m_object.init_inline<VT>(util::forward<Ts>(args)...);
            m_impl = &concrete_impl<VT>;
        }

        template<typename T, typename U, typename... Ts, typename VT = meta::Decay<T>>
        requires(concepts::ConstructibleFrom<T, std::initializer_list<U>&, Ts...> && is_callable_from<VT> &&
                 StoredInline<VT>)
        explicit Function(InPlaceType<T>, std::initializer_list<U> list, Ts&&... args) {
            m_object.init_inline<VT>(list, util::forward<Ts>(args)...);
            m_impl = &concrete_impl<VT>;
        }

        template<auto f, typename T, typename... Ts, typename VT = meta::Decay<T>>
        requires(concepts::ConstructibleFrom<T, Ts...> && is_callable_from<VT> && StoredInline<VT>)
        explicit Function(Nontype<f>, InPlaceType<T>, Ts&&... args) {
            m_object.init_inline<VT>(util::forward<Ts>(args)...);
            m_impl = &concrete_impl_for_bound_nontype<f, VT>;
        }

        template<auto f, typename T, typename U, typename... Ts, typename VT = meta::Decay<T>>
        requires(concepts::ConstructibleFrom<T, std::initializer_list<U>&, Ts...> && is_callable_from<VT> &&
                 StoredInline<VT>)
        explicit Function(Nontype<f>, InPlaceType<T>, std::initializer_list<U> list, Ts&&... args) {
            m_object.init_inline<VT>(list, util::forward<Ts>(args)...);
            m_impl = &concrete_impl_for_bound_nontype<f, VT>;
        }

        ~Function() = default;

        Function& operator=(nullptr_t) {
            m_object.reset();
            m_impl = nullptr;
            return *this;
        }
        Function& operator=(Function const&) = delete;
        Function& operator=(Function&& other) {
            m_object = util::move(other.m_object);
            m_impl = util::exchange(other.m_impl, nullptr);
            return *this;
        }

        template<typename F>
        requires(concepts::ConstructibleFrom<Function, F>)
        Function& operator=(F&& value) {
            auto new_value = Function(util::forward<F>(value));
            util::swap(*this, new_value);
            return *this;
        }

        R operator()(Args... args) noexcept(is_noexcept)
        requires(!is_const && !is_lvalue && !is_rvalue)
        {
            DI_ASSERT(m_impl);
            return m_impl(util::addressof(m_object), util::forward<Args>(args)...);
        }

        R operator()(Args... args) const noexcept(is_noexcept)
        requires(is_const && !is_lvalue && !is_rvalue)
        {
            DI_ASSERT(m_impl);
            return m_impl(util::addressof(m_object), util::forward<Args>(args)...);
        }

        R operator()(Args... args) & noexcept(is_noexcept)
        requires(!is_const && is_lvalue)
        {
            DI_ASSERT(m_impl);
            return m_impl(util::addressof(m_object), util::forward<Args>(args)...);
        }

        R operator()(Args... args) const& noexcept(is_noexcept)
        requires(is_const && is_lvalue)
        {
            DI_ASSERT(m_impl);
            return m_impl(util::addressof(m_object), util::forward<Args>(args)...);
        }

        R operator()(Args... args) && noexcept(is_noexcept)
        requires(!is_const && is_rvalue)
        {
            DI_ASSERT(m_impl);
            return m_impl(util::addressof(m_object), util::forward<Args>(args)...);
        }

        R operator()(Args... args) const&& noexcept(is_noexcept)
        requires(is_const && is_rvalue)
        {
            DI_ASSERT(m_impl);
            return m_impl(util::addressof(m_object), util::forward<Args>(args)...);
        }

        explicit operator bool() const { return m_impl != nullptr; }

    private:
        friend bool operator==(Function const& a, nullptr_t) { return !bool(a); }

        ErasedObject m_object {};
        ErasedFunctionPointer m_impl { nullptr };
    };

    template<typename Signature>
    struct MakeFunction {
    private:
        using Function = function_ns::Function<Signature>;

    public:
        template<typename F, typename VT = meta::Decay<F>>
        requires(!concepts::SameAs<meta::RemoveCVRef<F>, Function> &&
                 !concepts::InstanceOf<meta::RemoveCVRef<F>, InPlaceType> && Function::template is_callable_from<VT>)
        Function operator()(F&& object) const {
            Function result;
            if constexpr (concepts::MemberPointer<VT> ||
                          concepts::InstanceOf<meta::RemoveCVRef<F>, function_ns::Function>) {
                if (object == nullptr) {
                    return result;
                }
            }
            if constexpr (StoredInline<VT>) {
                result.m_object.template init_inline<VT>(util::forward<F>(object));
            } else {
                result.m_object.template init_out_of_line_infallible<VT>(util::forward<F>(object));
            }
            result.m_impl = &Function::template concrete_impl<VT>;
            return result;
        }

        template<auto f, typename T, typename VT = meta::Decay<T>>
        requires(concepts::ConstructibleFrom<VT, T> && Function::template is_callable_as_if_from<f, VT>)
        Function operator()(Nontype<f>, T&& object) const {
            Function result;
            if constexpr (StoredInline<VT>) {
                result.m_object.template init_inline<VT>(util::forward<T>(object));
            } else {
                result.m_object.template init_out_of_line_infallible<VT>(util::forward<T>(object));
            }
            result.m_impl = &Function::template concrete_impl_for_bound_nontype<f, VT>;
            return result;
        }

        template<typename T, typename... Ts, typename VT = meta::Decay<T>>
        requires(concepts::ConstructibleFrom<T, Ts...> && Function::template is_callable_from<VT>)
        Function operator()(InPlaceType<T>, Ts&&... args) const {
            Function result;
            if constexpr (StoredInline<VT>) {
                result.m_object.template init_inline<VT>(util::forward<Ts>(args)...);
            } else {
                result.m_object.template init_out_of_line_infallible<VT>(util::forward<Ts>(args)...);
            }
            result.m_impl = &Function::template concrete_impl<VT>;
            return result;
        }

        template<typename T, typename U, typename... Ts, typename VT = meta::Decay<T>>
        requires(concepts::ConstructibleFrom<T, std::initializer_list<U>&, Ts...> &&
                 Function::template is_callable_from<VT>)
        Function operator()(InPlaceType<T>, std::initializer_list<U> list, Ts&&... args) const {
            Function result;
            if constexpr (StoredInline<VT>) {
                result.m_object.template init_inline<VT>(list, util::forward<Ts>(args)...);
            } else {
                result.m_object.template init_out_of_line_infallible<VT>(list, util::forward<Ts>(args)...);
            }
            result.m_impl = &Function::template concrete_impl<VT>;
            return result;
        }

        template<auto f, typename T, typename... Ts, typename VT = meta::Decay<T>>
        requires(concepts::ConstructibleFrom<T, Ts...> && Function::template is_callable_from<VT>)
        Function operator()(Nontype<f>, InPlaceType<T>, Ts&&... args) const {
            Function result;
            if constexpr (StoredInline<VT>) {
                result.m_object.template init_inline<VT>(util::forward<Ts>(args)...);
            } else {
                result.m_object.template init_out_of_line_infallible<VT>(util::forward<Ts>(args)...);
            }
            result.m_impl = &Function::template concrete_impl_for_bound_nontype<f, VT>;
            return result;
        }

        template<auto f, typename T, typename U, typename... Ts, typename VT = meta::Decay<T>>
        requires(concepts::ConstructibleFrom<T, std::initializer_list<U>&, Ts...> &&
                 Function::template is_callable_from<VT> && StoredInline<VT>)
        Function operator()(Nontype<f>, InPlaceType<T>, std::initializer_list<U> list, Ts&&... args) const {
            Function result;
            if constexpr (StoredInline<VT>) {
                result.m_object.template init_inline<VT>(list, util::forward<Ts>(args)...);
            } else {
                result.m_object.template init_out_of_line_infallible<VT>(list, util::forward<Ts>(args)...);
            }
            result.m_impl = &Function::template concrete_impl_for_bound_nontype<f, VT>;
            return result;
        }
    };

    template<typename Signature>
    struct TryMakeFunction {
    private:
        using Function = function_ns::Function<Signature>;

    public:
        template<typename F, typename VT = meta::Decay<F>>
        requires(!concepts::SameAs<meta::RemoveCVRef<F>, Function> &&
                 !concepts::InstanceOf<meta::RemoveCVRef<F>, InPlaceType> && Function::template is_callable_from<VT>)
        Result<Function> operator()(F&& object) const {
            Function result;
            if constexpr (concepts::MemberPointer<VT> ||
                          concepts::InstanceOf<meta::RemoveCVRef<F>, function_ns::Function>) {
                if (object == nullptr) {
                    return result;
                }
            }
            if constexpr (StoredInline<VT>) {
                result.m_object.template init_inline<VT>(util::forward<F>(object));
            } else {
                DI_TRY(result.m_object.template init_out_of_line_fallible<VT>(util::forward<F>(object)));
            }
            result.m_impl = &Function::template concrete_impl<VT>;
            return result;
        }

        template<auto f, typename T, typename VT = meta::Decay<T>>
        requires(concepts::ConstructibleFrom<VT, T> && Function::template is_callable_as_if_from<f, VT>)
        Result<Function> operator()(Nontype<f>, T&& object) const {
            Function result;
            if constexpr (StoredInline<VT>) {
                result.m_object.template init_inline<VT>(util::forward<T>(object));
            } else {
                DI_TRY(result.m_object.template init_out_of_line_fallible<VT>(util::forward<T>(object)));
            }
            result.m_impl = &Function::template concrete_impl_for_bound_nontype<f, VT>;
            return result;
        }

        template<typename T, typename... Ts, typename VT = meta::Decay<T>>
        requires(concepts::ConstructibleFrom<T, Ts...> && Function::template is_callable_from<VT>)
        Result<Function> operator()(InPlaceType<T>, Ts&&... args) const {
            Function result;
            if constexpr (StoredInline<VT>) {
                result.m_object.template init_inline<VT>(util::forward<Ts>(args)...);
            } else {
                DI_TRY(result.m_object.template init_out_of_line_fallible<VT>(util::forward<Ts>(args)...));
            }
            result.m_impl = &Function::template concrete_impl<VT>;
            return result;
        }

        template<typename T, typename U, typename... Ts, typename VT = meta::Decay<T>>
        requires(concepts::ConstructibleFrom<T, std::initializer_list<U>&, Ts...> &&
                 Function::template is_callable_from<VT>)
        Result<Function> operator()(InPlaceType<T>, std::initializer_list<U> list, Ts&&... args) const {
            Function result;
            if constexpr (StoredInline<VT>) {
                result.m_object.template init_inline<VT>(list, util::forward<Ts>(args)...);
            } else {
                DI_TRY(result.m_object.template init_out_of_line_fallible<VT>(list, util::forward<Ts>(args)...));
            }
            result.m_impl = &Function::template concrete_impl<VT>;
            return result;
        }

        template<auto f, typename T, typename... Ts, typename VT = meta::Decay<T>>
        requires(concepts::ConstructibleFrom<T, Ts...> && Function::template is_callable_from<VT>)
        Result<Function> operator()(Nontype<f>, InPlaceType<T>, Ts&&... args) const {
            Function result;
            if constexpr (StoredInline<VT>) {
                result.m_object.template init_inline<VT>(util::forward<Ts>(args)...);
            } else {
                DI_TRY(result.m_object.template init_out_of_line_fallible<VT>(util::forward<Ts>(args)...));
            }
            result.m_impl = &Function::template concrete_impl_for_bound_nontype<f, VT>;
            return result;
        }

        template<auto f, typename T, typename U, typename... Ts, typename VT = meta::Decay<T>>
        requires(concepts::ConstructibleFrom<T, std::initializer_list<U>&, Ts...> &&
                 Function::template is_callable_from<VT> && StoredInline<VT>)
        Result<Function> operator()(Nontype<f>, InPlaceType<T>, std::initializer_list<U> list, Ts&&... args) const {
            Function result;
            if constexpr (StoredInline<VT>) {
                result.m_object.template init_inline<VT>(list, util::forward<Ts>(args)...);
            } else {
                DI_TRY(result.m_object.template init_out_of_line_fallible<VT>(list, util::forward<Ts>(args)...));
            }
            result.m_impl = &Function::template concrete_impl_for_bound_nontype<f, VT>;
            return result;
        }
    };
}

using function_ns::Function;

template<concepts::LanguageFunction T>
constexpr inline auto make_function = function_ns::MakeFunction<T> {};

template<concepts::LanguageFunction T>
constexpr inline auto try_make_function = function_ns::TryMakeFunction<T> {};
}
