#pragma once

#include "di/meta/core.h"
#include "di/types/integers.h"
#include "di/types/nullptr_t.h"
#include "di/util/initializer_list.h"

namespace di::concepts {
namespace detail {
    template<typename T>
    constexpr inline bool const_helper = false;

    template<typename T>
    constexpr inline bool const_helper<T const> = true;
}

template<typename T>
concept Const = detail::const_helper<T>;

namespace detail {
    template<typename T>
    constexpr inline bool lvalue_reference_helper = false;

    template<typename T>
    constexpr inline bool lvalue_reference_helper<T&> = true;

}

template<typename T>
concept LValueReference = detail::lvalue_reference_helper<T>;

template<typename T>
concept ConstLValueReference = LValueReference<T> && Const<meta::RemoveReference<T>>;

namespace detail {
    template<typename T>
    constexpr inline bool rvalue_reference_helper = false;

    template<typename T>
    constexpr inline bool rvalue_reference_helper<T&&> = true;
}

template<typename T>
concept RValueReference = detail::rvalue_reference_helper<T>;

template<typename T>
concept MutableRValueReference = RValueReference<T> && (!Const<meta::RemoveReference<T>>);

template<typename T>
concept Reference = LValueReference<T> || RValueReference<T>;

namespace detail {
    template<typename T>
    constexpr inline bool pointer_helper = false;

    template<typename T>
    constexpr inline bool pointer_helper<T*> = true;
}

template<typename T>
concept Pointer = detail::pointer_helper<meta::RemoveCV<T>>;

template<typename T>
concept LanguageFunction = (!Const<T const> && !Reference<T>);
}

namespace di::meta {
namespace detail {
    template<typename T>
    struct LanguageFunctionReturnHelper;

    template<typename R, typename... Args>
    struct LanguageFunctionReturnHelper<R(Args...)> : TypeConstant<R> {};
}

template<concepts::LanguageFunction Fun>
using LanguageFunctionReturn = Type<detail::LanguageFunctionReturnHelper<Fun>>;

template<typename R>
struct IsFunctionTo {
    template<typename T>
    using Invoke = Constexpr<concepts::LanguageFunction<T> && concepts::SameAs<R, LanguageFunctionReturn<T>>>;
};
}

namespace di::concepts {
namespace detail {
    template<typename T>
    constexpr inline bool bounded_language_array_helper = false;

    template<typename T, usize N>
    constexpr inline bool bounded_language_array_helper<T[N]> = true;
}

template<typename T>
concept BoundedLanguageArray = detail::bounded_language_array_helper<T>;

namespace detail {
    template<typename T>
    constexpr inline bool unbounded_language_array_helper = false;

    template<typename T>
    constexpr inline bool unbounded_language_array_helper<T[]> = true;
}

template<typename T>
concept UnboundedLanguageArray = detail::unbounded_language_array_helper<T>;

template<typename T>
concept LanguageArray = BoundedLanguageArray<T> || UnboundedLanguageArray<T>;
}

namespace di::meta {
namespace detail {
    template<typename T>
    struct RemoveExtentHelper : TypeConstant<T> {};

    template<typename T>
    struct RemoveExtentHelper<T[]> : TypeConstant<T> {};

    template<typename T, types::size_t N>
    struct RemoveExtentHelper<T[N]> : TypeConstant<T> {};
}

template<typename T>
using RemoveExtent = Type<detail::RemoveExtentHelper<T>>;

namespace detail {
    template<typename T>
    struct RemoveAllExtentsHelper : TypeConstant<T> {};

    template<concepts::LanguageArray T>
    struct RemoveAllExtentsHelper<T> : RemoveAllExtentsHelper<RemoveExtent<T>> {};
}

template<typename T>
using RemoveAllExtents = Type<detail::RemoveAllExtentsHelper<T>>;

template<typename T>
constexpr inline auto ArrayRank = 0ZU;

template<typename T>
constexpr inline auto ArrayRank<T[]> = 1 + ArrayRank<T>;

template<typename T, usize N>
constexpr inline auto ArrayRank<T[N]> = 1 + ArrayRank<T>;

template<typename T, types::size_t level = 0>
constexpr inline auto Extent = 0ZU;

template<typename T, usize level>
constexpr inline auto Extent<T[], level> = Extent<T, level - 1>;

template<typename T, usize size>
constexpr inline auto Extent<T[size], 0> = size;

template<typename T, usize size, usize level>
constexpr inline auto Extent<T[size], level> = Extent<T, level - 1>;
}

namespace di::concepts {
namespace detail {
    template<typename T>
    constexpr inline bool member_pointer_helper = false;

    template<typename T, typename U>
    constexpr inline bool member_pointer_helper<T U::*> = true;
}

template<typename T>
concept MemberPointer = detail::member_pointer_helper<meta::RemoveCV<T>>;
}

namespace di::meta {
namespace detail {
    template<typename T>
    struct MemberPointerValueHelper {};

    template<typename Value, typename Class>
    struct MemberPointerValueHelper<Value Class::*> : TypeConstant<Value> {};
}

template<concepts::MemberPointer T>
using MemberPointerValue = Type<detail::MemberPointerValueHelper<RemoveCV<T>>>;

namespace detail {
    template<typename T>
    struct MemberPointerClassHelper {};

    template<typename Value, typename Class>
    struct MemberPointerClassHelper<Value Class::*> : TypeConstant<Class> {};
}

template<concepts::MemberPointer T>
using MemberPointerClass = Type<detail::MemberPointerClassHelper<RemoveCV<T>>>;
}

namespace di::concepts {
template<typename T>
concept MemberFunctionPointer = MemberPointer<T> && LanguageFunction<meta::MemberPointerValue<T>>;

template<typename T>
concept MemberObjectPointer = (MemberPointer<T> && !MemberFunctionPointer<T>);

template<typename T>
concept NullPointer = SameAs<meta::RemoveCV<T>, nullptr_t>;

template<typename T>
concept InitializerList = InstanceOf<meta::RemoveCVRef<T>, std::initializer_list>;

template<typename T>
concept Aggregate = __is_aggregate(meta::RemoveCV<T>);

template<typename T>
concept Integer = OneOf<meta::RemoveCV<T>, signed char, char, short, int, long, long long, unsigned char,
                        unsigned short, unsigned int, unsigned long, unsigned long long
#ifdef DI_HAVE_128_BIT_INTEGERS
                        ,
                        types::i128, types::u128
#endif
                        >;

template<typename T>
concept Integral = OneOf<meta::RemoveCV<T>, bool, signed char, char, short, int, long, long long, unsigned char,
                         unsigned short, unsigned int, unsigned long, unsigned long long,
#ifdef DI_HAVE_128_BIT_INTEGERS
                         types::i128, types::u128,
#endif
                         char8_t, char16_t, char32_t>;

template<typename T>
concept FloatingPoint = OneOf<meta::RemoveCV<T>, float, double, long double>;

template<typename T>
concept Arithmetic = Integral<T> || FloatingPoint<T>;

template<typename T>
concept Signed = Arithmetic<T> && (T(-1) < T(0));

template<typename T>
concept SignedInteger = Integer<T> && Signed<T>;

template<typename T>
concept UnsignedInteger = Integer<T> && !SignedInteger<T>;

template<typename T>
concept SignedIntegral = Integral<T> && Signed<T>;

template<typename T>
concept UnsignedIntegral = Integral<T> && !SignedIntegral<T>;

template<typename T>
concept Class = __is_class(T);

template<typename T>
concept Union = __is_union(T);

template<typename T>
concept Enum = __is_enum(T);

template<typename T>
concept IntegralOrEnum = Integral<T> || Enum<T>;
}

namespace di::meta {
template<concepts::Enum T>
using UnderlyingType = __underlying_type(T);

namespace detail {
    template<typename T>
    struct MakeSignedHelper {};

    template<concepts::Enum T>
    struct MakeSignedHelper<T> : MakeSignedHelper<UnderlyingType<T>> {};

    template<concepts::SignedIntegral T>
    struct MakeSignedHelper<T> : TypeConstant<T> {};

    template<>
    struct MakeSignedHelper<unsigned char> : TypeConstant<char> {};

    template<>
    struct MakeSignedHelper<unsigned short> : TypeConstant<short> {};

    template<>
    struct MakeSignedHelper<unsigned int> : TypeConstant<int> {};

    template<>
    struct MakeSignedHelper<unsigned long> : TypeConstant<long> {};

    template<>
    struct MakeSignedHelper<unsigned long long> : TypeConstant<long long> {};

#ifdef DI_HAVE_128_BIT_INTEGERS
    template<>
    struct MakeSignedHelper<types::u128> : TypeConstant<types::i128> {};
#endif

    template<>
    struct MakeSignedHelper<char8_t> : TypeConstant<types::i8> {};

    template<>
    struct MakeSignedHelper<char16_t> : TypeConstant<types::i16> {};

    template<>
    struct MakeSignedHelper<char32_t> : TypeConstant<types::i32> {};

    template<>
    struct MakeSignedHelper<float> : TypeConstant<float> {};

    template<>
    struct MakeSignedHelper<double> : TypeConstant<double> {};
}

template<typename T>
using MakeSigned = detail::MakeSignedHelper<RemoveCV<T>>::Type;

namespace detail {
    template<typename T>
    struct MakeUnsignedHelper {};

    template<concepts::Enum T>
    struct MakeUnsignedHelper<T> : MakeUnsignedHelper<UnderlyingType<T>> {};

    template<concepts::UnsignedIntegral T>
    struct MakeUnsignedHelper<T> : TypeConstant<T> {};

    template<>
    struct MakeUnsignedHelper<char> : TypeConstant<unsigned char> {};

    template<>
    struct MakeUnsignedHelper<signed char> : TypeConstant<unsigned char> {};

    template<>
    struct MakeUnsignedHelper<short> : TypeConstant<unsigned short> {};

    template<>
    struct MakeUnsignedHelper<int> : TypeConstant<unsigned int> {};

    template<>
    struct MakeUnsignedHelper<long> : TypeConstant<unsigned long> {};

    template<>
    struct MakeUnsignedHelper<long long> : TypeConstant<unsigned long long> {};

#ifdef DI_HAVE_128_BIT_INTEGERS
    template<>
    struct MakeUnsignedHelper<types::i128> : TypeConstant<types::u128> {};
#endif

    template<>
    struct MakeUnsignedHelper<char8_t> : TypeConstant<types::u8> {};

    template<>
    struct MakeUnsignedHelper<char16_t> : TypeConstant<types::u16> {};

    template<>
    struct MakeUnsignedHelper<char32_t> : TypeConstant<types::u32> {};
}

template<typename T>
using MakeUnsigned = detail::MakeUnsignedHelper<RemoveCV<T>>::Type;
}

namespace di::concepts {
template<typename T>
concept Scalar = (Arithmetic<T> || Enum<T> || Pointer<T> || MemberPointer<T> || NullPointer<T>);

template<typename T>
concept Object = Scalar<T> || LanguageArray<T> || Union<T> || Class<T>;

template<typename T, typename U>
concept BaseOf = __is_base_of(T, U);

template<typename T>
concept UniqueObjectRepresentation = __has_unique_object_representations(meta::RemoveCV<meta::RemoveAllExtents<T>>);
}

namespace di::meta {
template<typename T>
using AddConst = T const;

template<typename T>
using AddVolatile = T volatile;

template<typename T>
using AddCV = T const volatile;

namespace detail {
    template<typename T>
    struct AddLValueReferenceHelper : TypeConstant<T> {};

    template<typename T>
    requires(requires { typename TypeConstant<T&>; })
    struct AddLValueReferenceHelper<T> : TypeConstant<T&> {};
}

template<typename T>
using AddLValueReference = Type<detail::AddLValueReferenceHelper<T>>;

namespace detail {
    template<typename T>
    struct AddRValueReferenceHelper : TypeConstant<T> {};

    template<typename T>
    requires(requires { typename TypeConstant<T&&>; })
    struct AddRValueReferenceHelper<T> : TypeConstant<T&&> {};
}

template<typename T>
using AddRValueReference = Type<detail::AddRValueReferenceHelper<T>>;

namespace detail {
    template<typename T>
    struct AddPointerHelper : TypeConstant<T> {};

    template<typename T>
    requires(requires { typename TypeConstant<RemoveReference<T>*>; })
    struct AddPointerHelper<T> : TypeConstant<RemoveReference<T>*> {};
}

/// @brief This is a helper template which will convert reference types into their
/// corresponding pointer type, while also working for non-references.
///
/// For types which cannot be made into a pointer (function types), this does nothing.
template<typename T>
using AddPointer = Type<detail::AddPointerHelper<T>>;

namespace detail {
    template<typename T, typename U>
    struct LikeHelper : TypeConstant<U> {};

    template<typename T, typename U>
    struct LikeHelper<T const, U> : TypeConstant<U const> {};

    template<typename T, typename U>
    struct LikeHelper<T volatile, U> : TypeConstant<U volatile> {};

    template<typename T, typename U>
    struct LikeHelper<T const volatile, U> : TypeConstant<U const volatile> {};

    template<typename T, typename U>
    struct LikeHelper<T&, U> : TypeConstant<U&> {};

    template<typename T, typename U>
    struct LikeHelper<T const&, U> : TypeConstant<U const&> {};

    template<typename T, typename U>
    struct LikeHelper<T volatile&, U> : TypeConstant<U volatile&> {};

    template<typename T, typename U>
    struct LikeHelper<T const volatile&, U> : TypeConstant<U const volatile&> {};

    template<typename T, typename U>
    struct LikeHelper<T&&, U> : TypeConstant<U&&> {};

    template<typename T, typename U>
    struct LikeHelper<T const&&, U> : TypeConstant<U const&&> {};

    template<typename T, typename U>
    struct LikeHelper<T volatile&&, U> : TypeConstant<U volatile&&> {};

    template<typename T, typename U>
    struct LikeHelper<T const volatile&&, U> : TypeConstant<U const volatile&&> {};
}

template<typename T, typename U>
using Like = Type<detail::LikeHelper<T, U>>;

template<typename T>
using RemoveRValueReference = Conditional<concepts::RValueReference<T>, RemoveReference<T>, T>;

namespace detail {
    template<typename T>
    struct RemovePointerHelper : TypeConstant<T> {};

    template<typename T>
    struct RemovePointerHelper<T*> : TypeConstant<T> {};

    template<typename T>
    struct RemovePointerHelper<T* const> : TypeConstant<T> {};

    template<typename T>
    struct RemovePointerHelper<T* volatile> : TypeConstant<T> {};

    template<typename T>
    struct RemovePointerHelper<T* const volatile> : TypeConstant<T> {};
}

template<typename T>
using RemovePointer = Type<detail::RemovePointerHelper<T>>;

namespace detail {
    template<typename T>
    struct RemoveFunctionQualifiersHelper;

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...)> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) const> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) volatile> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) const volatile> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...)&> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) const&> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) volatile&> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) const volatile&> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) &&> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) const&&> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) volatile&&> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) const volatile&&> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) noexcept> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) const noexcept> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) volatile noexcept> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) const volatile noexcept> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) & noexcept> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) const & noexcept> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) volatile & noexcept> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) const volatile & noexcept> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) && noexcept> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) const && noexcept> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) volatile && noexcept> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) const volatile && noexcept> : TypeConstant<R(Args...)> {};
}

template<typename T>
using RemoveFunctionQualifiers = Type<detail::RemoveFunctionQualifiersHelper<T>>;

template<typename T>
using Decay = Conditional<
    concepts::LanguageArray<RemoveReference<T>>, RemoveExtent<RemoveReference<T>>*,
    Conditional<concepts::LanguageFunction<RemoveReference<T>>, AddPointer<RemoveReference<T>>, RemoveCVRef<T>>>;
}
