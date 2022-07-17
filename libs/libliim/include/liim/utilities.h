#pragma once

#include <liim/forward.h>
#include <liim/initializer_list.h>
#include <stddef.h>
#include <sys/types.h>

#if !defined(__is_libc) && !defined(__is_libk)
#include <memory>
#include <new>
#include <stdlib.h>
#else
#include <bits/malloc.h>
#include <stddef.h>

inline void* operator new(size_t, void* p) {
    return p;
}
inline void* operator new[](size_t, void* p) {
    return p;
}

inline void operator delete(void*, void*) {};
inline void operator delete[](void*, void*) {};

extern "C" {
void* malloc(size_t n);
void* realloc(void* p, size_t n);
void free(void* p);
void* calloc(size_t n, size_t sz);
}
#endif

namespace LIIM {

typedef __SIZE_TYPE__ size_t;

struct TrueType {
    static constexpr bool value = true;
};

struct FalseType {
    static constexpr bool value = false;
};

template<typename T>
struct TypeIdentity {
    typedef T type;
};

template<bool B, typename T = void>
struct EnableIf {};
template<typename T>
struct EnableIf<true, T> {
    typedef T type;
};

template<typename A, typename B>
struct IsSame : FalseType {};

template<typename T>
struct IsSame<T, T> : TrueType {};

template<typename T, typename U>
concept SameAs = IsSame<T, U>::value;

template<typename T>
struct IsArray : FalseType {};
template<typename T>
struct IsArray<T[]> : TrueType {};
template<typename T, size_t N>
struct IsArray<T[N]> : TrueType {};

template<typename T>
struct IsPointer : FalseType {};
template<typename T>
struct IsPointer<T*> : TrueType {};

template<typename T>
struct IsReference : FalseType {};
template<typename T>
struct IsReference<T&> : TrueType {};
template<typename T>
struct IsReference<T&&> : TrueType {};

template<typename T>
struct IsConst : FalseType {};
template<typename T>
struct IsConst<const T> : TrueType {};

template<typename T>
struct IsFunction {
    enum { value = !IsConst<const T>::value && !IsReference<T>::value };
};

template<typename T>
struct IsRValueReference : FalseType {};
template<typename T>
struct IsRValueReference<T&&> : TrueType {};

template<typename T>
struct IsLValueReference : FalseType {};
template<typename T>
struct IsLValueReference<T&> : TrueType {};

template<typename T>
struct RemovePointer {
    typedef T type;
};
template<typename T>
struct RemovePointer<T*> {
    typedef T type;
};
template<typename T>
struct RemovePointer<T* const> {
    typedef T type;
};
template<typename T>
struct RemovePointer<T* volatile> {
    typedef T type;
};
template<typename T>
struct RemovePointer<T* const volatile> {
    typedef T type;
};

template<typename T>
struct RemoveReference {
    typedef T type;
};
template<typename T>
struct RemoveReference<T&> {
    typedef T type;
};
template<typename T>
struct RemoveReference<T&&> {
    typedef T type;
};

template<typename T>
struct RemoveExtent {
    typedef T type;
};
template<typename T>
struct RemoveExtent<T[]> {
    typedef T type;
};
template<typename T, size_t N>
struct RemoveExtent<T[N]> {
    typedef T type;
};

template<typename T>
struct RemoveConst {
    typedef T type;
};
template<typename T>
struct RemoveConst<const T> {
    typedef T type;
};

template<typename T>
struct RemoveVolatile {
    typedef T type;
};
template<typename T>
struct RemoveVolatile<volatile T> {
    typedef T type;
};

template<typename T>
struct RemoveCV {
    typedef typename RemoveConst<typename RemoveVolatile<T>::type>::type type;
};

template<class T>
struct RemoveCVRef {
    typedef typename RemoveCV<typename RemoveReference<T>::type>::type type;
};

template<typename T>
struct IsVoid : IsSame<void, typename RemoveCV<T>::type> {};

template<typename T, typename... Types>
struct IsOneOf;

template<typename T, typename First, typename... Rest>
struct IsOneOf<T, First, Rest...> {
    static constexpr bool value = IsSame<T, First>::value || IsOneOf<T, Rest...>::value;
};

template<typename T>
struct IsOneOf<T> {
    static constexpr bool value = false;
};

template<typename T, typename... Args>
concept OneOf = IsOneOf<T, Args...>::value;

template<typename T>
struct IsIntegral
    : IsOneOf<typename RemoveCV<T>::type, bool, char, short, int, long, long long, unsigned char, unsigned short, unsigned int,
              unsigned long, unsigned long long> {};

template<typename T>
concept Integral = IsIntegral<T>::value;

template<typename T>
struct IsUnsigned
    : IsOneOf<typename RemoveCV<T>::type, bool, unsigned char, unsigned short, unsigned int, unsigned long, unsigned long long> {};

template<typename T>
concept UnsignedIntegral = IsUnsigned<T>::value;

template<typename T>
struct IsSigned : IsOneOf<typename RemoveCV<T>::type, char, short, int, long, long long> {};

template<typename T>
concept SignedIntegral = IsSigned<T>::value;

template<typename T>
struct MakeUnsigned;

template<UnsignedIntegral T>
struct MakeUnsigned<T> {
    using type = T;
};

template<>
struct MakeUnsigned<char> {
    using type = unsigned char;
};

template<>
struct MakeUnsigned<short> {
    using type = unsigned short;
};

template<>
struct MakeUnsigned<int> {
    using type = unsigned int;
};

template<>
struct MakeUnsigned<long> {
    using type = unsigned long;
};

template<>
struct MakeUnsigned<long long> {
    using type = unsigned long long;
};

namespace details {
    template<class T>
    struct IsMemberFunctionPointerImpl : FalseType {};
    template<class T, class U>
    struct IsMemberFunctionPointerImpl<T U::*> : IsFunction<T> {};

    template<class T>
    struct IsMemberPointerImpl : FalseType {};
    template<class T, class U>
    struct IsMemberPointerImpl<T U::*> : TrueType {};
}

template<class T>
struct IsMemberFunctionPointer : details::IsMemberFunctionPointerImpl<typename RemoveCV<T>::type> {};
template<class T>
struct IsMemberPointer : details::IsMemberPointerImpl<typename RemoveCV<T>::type> {};

template<class T>
struct IsMemberObjectPointer {
    enum { value = IsMemberPointer<T>::value && !IsMemberFunctionPointer<T>::value };
};

namespace details {
    template<typename T>
    auto try_add_pointer(int) -> TypeIdentity<typename RemoveReference<T>::type*>;
    template<typename T>
    auto try_add_pointer(...) -> TypeIdentity<T>;
}

template<typename T>
struct AddPointer : decltype(details::try_add_pointer<T>(0)) {};

template<bool Z, typename A, typename B>
struct Conditional {
    typedef B type;
};
template<typename A, typename B>
struct Conditional<true, A, B> {
    typedef A type;
};

template<typename T>
T&& declval() {
    return 0;
}

namespace details {
    template<typename>
    using TrueTypeFor = TrueType;

    template<typename T>
    auto test_returnable(int) -> TrueTypeFor<T()>;
    template<typename T>
    auto test_returnable(...) -> FalseType;

    template<typename From, typename To>
    auto test_nonvoid_convertible(int) -> TrueTypeFor<decltype(LIIM::declval<void (&)(To)>()(LIIM::declval<From>()))>;
    template<typename From, typename To>
    auto test_nonvoid_convertible(...) -> FalseType;
}

template<typename From, typename To>
struct IsConvertible {
    static constexpr bool value =
        (decltype(details::test_returnable<To>(0))::value && decltype(details::test_nonvoid_convertible<From, To>(0))::value);
};

template<typename T, typename... Args>
concept ConstructibleFrom = requires {
    T(declval<Args>()...);
};

template<typename T>
struct Decay {
private:
    typedef typename RemoveReference<T>::type U;

public:
    typedef typename Conditional<
        IsArray<U>::value, typename RemoveExtent<U>::type*,
        typename Conditional<IsFunction<U>::value, typename AddPointer<U>::type, typename RemoveCV<U>::type>::type>::type type;
};

template<typename T>
using decay_t = typename Decay<T>::type;
}

namespace std {
typedef decltype(nullptr) nullptr_t;

#if defined(__is_libc) || defined(__is_libk)
template<typename T>
inline constexpr T&& forward(typename LIIM::TypeIdentity<T>::type& param) {
    return static_cast<T&&>(param);
}

template<typename T>
inline constexpr typename LIIM::RemoveReference<T>::type&& move(T&& arg) {
    return static_cast<typename LIIM::RemoveReference<T>::type&&>(arg);
}

template<typename T, typename... Args>
constexpr T* construct_at(T* location, Args&&... args) {
    return ::new (const_cast<void*>(static_cast<const volatile void*>(location))) T(std::forward<Args>(args)...);
}
#endif
}

namespace LIIM {
using std::construct_at;
using std::forward;
using std::move;

namespace Detail {
    template<typename T>
    struct IsResultHelper {
        using Type = T;
        constexpr static bool value = false;
    };

    template<typename T, typename U>
    struct IsResultHelper<Result<T, U>> {
        using Type = T;
        constexpr static bool value = true;
    };

    template<typename T>
    struct IsOptionHelper {
        using Type = T;
        constexpr static bool value = false;
    };

    template<typename T>
    struct IsOptionHelper<Option<T>> {
        using Type = T;
        constexpr static bool value = true;
    };
}

template<typename T>
concept IsResult = Detail::IsResultHelper<decay_t<T>>::value;

template<typename T>
concept IsOption = Detail::IsOptionHelper<decay_t<T>>::value;

template<typename Res, typename T>
concept ResultOf = IsResult<Res> && SameAs<typename decay_t<Res>::ValueType, T>;

template<typename Opt, typename T>
concept OptionOf = IsOption<Opt> && SameAs<typename decay_t<Opt>::ValueType, T>;

template<typename T>
using ResultValueType = Detail::IsResultHelper<decay_t<T>>::Type;

template<typename T>
using OptionValueType = Detail::IsOptionHelper<decay_t<T>>::Type;

namespace Detail {
    template<typename T>
    struct UnwrapResultHelper {
        using Type = T;
    };

    template<IsResult T>
    struct UnwrapResultHelper<T> {
        using Type = decay_t<T>::ValueType;
    };
}

template<typename T>
using UnwrapResult = Detail::UnwrapResultHelper<T>::Type;

namespace Detail {
    template<typename T, typename U>
    struct LikeHelper {
        using Type = U;
    };

    template<typename T, typename U>
    struct LikeHelper<T&, U> {
        using Type = U&;
    };

    template<typename T, typename U>
    struct LikeHelper<const T&, U> {
        using Type = const U&;
    };

    template<typename T, typename U>
    struct LikeHelper<T&&, U> {
        using Type = U&&;
    };

    template<typename T, typename U>
    struct LikeHelper<const T&&, U> {
        using Type = const U&&;
    };
}

template<typename T, typename U>
using Like = Detail::LikeHelper<T, U>::Type;

struct Void {};

template<auto...>
constexpr bool always_false = false;

namespace Detail {
    template<typename T>
    struct WrapVoidHelper {
        using Type = Conditional<IsVoid<T>::value, Void, T>::type;
    };

    template<typename T>
    struct UnwrapVoidHelper {
        using Type = Conditional<SameAs<T, Void>, void, T>::type;
    };
}

template<typename T>
using WrapVoid = Detail::WrapVoidHelper<T>::Type;

template<typename T>
using UnwrapVoid = Detail::WrapVoidHelper<T>::Type;

template<size_t... Ints>
struct IndexSequence {
    static constexpr int size() { return sizeof...(Ints); }
    typedef IndexSequence type;
    typedef size_t value_type;
};

namespace details {
    template<typename T>
    using Invoke = typename T::type;

    template<typename S1, typename S2>
    struct ConcatImpl;

    template<size_t... I1, size_t... I2>
    struct ConcatImpl<IndexSequence<I1...>, IndexSequence<I2...>> : IndexSequence<I1..., (sizeof...(I1) + I2)...> {};

    template<typename S1, typename S2>
    using Concat = Invoke<ConcatImpl<S1, S2>>;

    template<size_t N>
    struct GenSeqImpl;
    template<size_t N>
    using GenSeq = Invoke<GenSeqImpl<N>>;
    template<size_t N>
    struct GenSeqImpl : Concat<GenSeq<N / 2>, GenSeq<N - N / 2>> {};

    template<>
    struct GenSeqImpl<0> : IndexSequence<> {};
    template<>
    struct GenSeqImpl<1> : IndexSequence<0> {};
}

template<size_t N>
using make_index_sequence = details::GenSeq<N>;

template<typename T>
struct ReferenceWrapper;

template<typename T>
struct IsReferenceWrapper : FalseType {};
template<typename U>
struct IsReferenceWrapper<ReferenceWrapper<U>> : TrueType {};

template<typename T>
struct IsUnion : FalseType {};

namespace details {
    template<bool b>
    struct BoolType {
        enum { value = b };
    };

    template<class T>
    BoolType<!IsUnion<T>::value> test(int T::*);

    template<class>
    FalseType test(...);
}

template<class T>
struct IsClass : decltype(details::test<T>(nullptr)) {};

namespace details {
    template<typename B>
    TrueType test_pre_ptr_convertible(const volatile B*);
    template<typename>
    FalseType test_pre_ptr_convertible(const volatile void*);

    template<typename, typename>
    auto test_is_pre_base_of(...) -> TrueType;
    template<typename B, typename D>
    auto test_is_pre_base_of(int) -> decltype(test_pre_ptr_convertible<B>(static_cast<D*>(nullptr)));
}

template<typename Base, typename Derived>
struct IsBaseOf {
    static constexpr bool value =
        IsClass<Base>::value && IsClass<Derived>::value && decltype(details::test_is_pre_base_of<Base, Derived>(0))::value;
};

namespace details {
    template<typename T, typename Type, typename T1, typename... Args>
    constexpr decltype(auto) INVOKE(Type T::*f, T1&& t1, Args&&... args) {
        if constexpr (IsMemberFunctionPointer<decltype(f)>::value) {
            if constexpr (IsBaseOf<T, decay_t<T1>>::value)
                return (forward<T1>(t1).*f)(forward<Args>(args)...);
            else if constexpr (LIIM::IsReferenceWrapper<decay_t<T1>>::value)
                return (t1.get().*f)(forward<Args>(args)...);
            else
                return ((*forward<T1>(t1)).*f)(forward<Args>(args)...);
        } else {
            static_assert(IsMemberObjectPointer<decltype(f)>::value);
            static_assert(sizeof...(args) == 0);
            if constexpr (IsBaseOf<T, decay_t<T1>>::value)
                return forward<T1>(t1).*f;
            else if constexpr (LIIM::IsReferenceWrapper<decay_t<T1>>::value)
                return t1.get().*f;
            else
                return (*forward<T1>(t1)).*f;
        }
    }

    template<class F, class... Args>
    constexpr decltype(auto) INVOKE(F&& f, Args&&... args) {
        return forward<F>(f)(forward<Args>(args)...);
    }
}

template<typename F, typename... Args>
struct InvokeResult {
    using type = decltype(details::INVOKE(declval<F>(), declval<Args>()...));
};

template<class F, class... Args>
constexpr typename InvokeResult<F, Args...>::type invoke(F&& f, Args&&... args) {
    return details::INVOKE(forward<F>(f), forward<Args>(args)...);
}

namespace details {
    template<class T>
    constexpr T& FUN(T& t) {
        return t;
    }
    template<class T>
    void FUN(T&&) = delete;
}

template<typename T>
class ReferenceWrapper {
public:
    typedef T type;

    template<class U, class = decltype(details::FUN<T>(LIIM::declval<U>()),
                                       typename LIIM::EnableIf<!IsSame<ReferenceWrapper, typename RemoveCVRef<U>::type>::value>::type())>
    constexpr ReferenceWrapper(U&& u) : _ptr(addressof(details::FUN<T>(forward<U>(u)))) {}
    ReferenceWrapper(const ReferenceWrapper&) = default;

    ReferenceWrapper& operator=(const ReferenceWrapper&) = default;

    constexpr operator T&() const { return *_ptr; }
    constexpr T& get() const { return *_ptr; }

    template<typename... ArgTypes>
    constexpr typename InvokeResult<T&, ArgTypes...>::type operator()(ArgTypes&&... args) const {
        return invoke(get(), forward<ArgTypes>(args)...);
    }

private:
    T* _ptr;
};

template<typename T>
ReferenceWrapper(T&) -> ReferenceWrapper<T>;

template<typename Derived, typename Base>
concept DerivedFrom = IsBaseOf<Base, Derived>::value && IsConvertible<const volatile Derived*, const volatile Base*>::value;

template<typename T>
struct IsTriviallyRelocatable {
    constexpr static bool value = false;
};

template<Integral T>
struct IsTriviallyRelocatable<T> {
    constexpr static bool value = true;
};

template<typename T>
concept TriviallyRelocatable = IsTriviallyRelocatable<T>::value;

namespace Detail {
    template<bool... values>
    struct ConjunctionHelper {
        constexpr static bool value = true;
    };

    template<bool b, bool... values>
    struct ConjunctionHelper<b, values...> {
        constexpr static bool value = !b ? false : ConjunctionHelper<values...>::value;
    };

    template<bool... values>
    struct DisjunctionHelper {
        constexpr static bool value = false;
    };

    template<bool b, bool... values>
    struct DisjunctionHelper<b, values...> {
        constexpr static bool value = b ? true : DisjunctionHelper<values...>::value;
    };
}

template<bool... values>
constexpr inline bool conjunction = Detail::ConjunctionHelper<values...>::value;

template<bool... values>
constexpr inline bool disjunction = Detail::DisjunctionHelper<values...>::value;

template<class T>
struct in_place_type_t {
    explicit in_place_type_t() = default;
};
template<class T>
inline constexpr in_place_type_t<T> in_place_type {};

template<size_t I>
struct in_place_index_t {
    explicit in_place_index_t() = default;
};
template<size_t I>
inline constexpr in_place_index_t<I> in_place_index {};

struct piecewise_construct_t {
    explicit piecewise_construct_t() = default;
};
inline constexpr piecewise_construct_t piecewise_construct {};

template<typename U, typename T>
constexpr U bit_cast(const T& value) {
    return __builtin_bit_cast(U, value);
}

template<typename T>
constexpr void swap(T& a, T& b) {
    if (&a != &b) {
        T temp(move(a));
        a.~T();
        std::construct_at(&a, move(b));
        b.~T();
        std::construct_at(&b, move(temp));
    }
}

template<typename T, typename U = T>
constexpr T exchange(T& object, U&& new_value) {
    auto temp = LIIM::move(object);
    object = forward<U>(new_value);
    return temp;
}

template<typename T>
constexpr const T& clamp(const T& t, const T& min, const T& max) {
    if (t < min) {
        return min;
    } else if (t > max) {
        return max;
    }

    return t;
}

template<typename T>
constexpr const T& max(const T& a, const T& b) {
    return a >= b ? a : b;
}

template<typename T>
constexpr const T& min(const T& a, const T& b) {
    return a <= b ? a : b;
}

template<typename T>
constexpr T abs(T x);

template<UnsignedIntegral T>
constexpr T abs(T x) {
    return x;
}

template<SignedIntegral T>
constexpr T abs(T x) {
    return x < 0 ? -x : x;
}
}

using LIIM::abs;
using LIIM::bit_cast;
using LIIM::clamp;
using LIIM::conjunction;
using LIIM::disjunction;
using LIIM::exchange;
using LIIM::forward;
using LIIM::in_place_index;
using LIIM::in_place_index_t;
using LIIM::in_place_type;
using LIIM::in_place_type_t;
using LIIM::IsOption;
using LIIM::IsResult;
using LIIM::max;
using LIIM::min;
using LIIM::move;
using LIIM::OptionOf;
using LIIM::piecewise_construct;
using LIIM::piecewise_construct_t;
using LIIM::ResultOf;
using LIIM::SameAs;
using LIIM::swap;
using LIIM::TriviallyRelocatable;
