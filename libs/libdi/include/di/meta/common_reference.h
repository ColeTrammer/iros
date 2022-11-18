#pragma once

#include <di/concepts/lvalue_reference.h>
#include <di/concepts/rvalue_reference.h>
#include <di/meta/common_type.h>
#include <di/meta/like.h>
#include <di/meta/remove_cvref.h>

namespace di::meta {
template<typename T, typename U, template<typename> typename TQual, template<typename> typename UQual>
struct CustomCommonReference {};

namespace detail {
    template<typename T>
    T __get_value();

    template<typename T>
    struct ProjectQualifiers {
        template<typename U>
        using Type = Like<T, U>;
    };

    template<typename T, typename U, typename R>
    struct UnionCV : TypeConstant<R> {};

    template<typename T, typename U, typename R>
    struct UnionCV<T const, U, R> : TypeConstant<R const> {};

    template<typename T, typename U, typename R>
    struct UnionCV<T const, U const, R> : TypeConstant<R const> {};

    template<typename T, typename U, typename R>
    struct UnionCV<T const, U volatile, R> : TypeConstant<R const volatile> {};

    template<typename T, typename U, typename R>
    struct UnionCV<T const, U const volatile, R> : TypeConstant<R const volatile> {};

    template<typename T, typename U, typename R>
    struct UnionCV<T volatile, U, R> : TypeConstant<R volatile> {};

    template<typename T, typename U, typename R>
    struct UnionCV<T volatile, U const, R> : TypeConstant<R const volatile> {};

    template<typename T, typename U, typename R>
    struct UnionCV<T volatile, U volatile, R> : TypeConstant<R volatile> {};

    template<typename T, typename U, typename R>
    struct UnionCV<T volatile, U const volatile, R> : TypeConstant<R const volatile> {};

    template<typename T, typename U, typename R>
    struct UnionCV<T const volatile, U, R> : TypeConstant<R const volatile> {};

    template<typename T, typename U, typename R>
    struct UnionCV<T const volatile, U const, R> : TypeConstant<R const volatile> {};

    template<typename T, typename U, typename R>
    struct UnionCV<T const volatile, U volatile, R> : TypeConstant<R const volatile> {};

    template<typename T, typename U, typename R>
    struct UnionCV<T const volatile, U const volatile, R> : TypeConstant<R const volatile> {};

    template<typename T, typename U>
    struct SimpleCommonReference {};

    template<concepts::LValueReference T, concepts::LValueReference U>
    requires(
        requires {
            false ? util::declval<typename UnionCV<meta::RemoveReference<T>, meta::RemoveReference<U>, meta::RemoveReference<T>>::Type&>()
                  : util::declval<typename UnionCV<meta::RemoveReference<T>, meta::RemoveReference<U>, meta::RemoveReference<U>>::Type&>();
        })
    struct SimpleCommonReference<T, U>
        : TypeConstant<decltype(false ? util::declval<typename UnionCV<meta::RemoveReference<T>, meta::RemoveReference<U>,
                                                                       meta::RemoveReference<T>>::Type&>()
                                      : util::declval<typename UnionCV<meta::RemoveReference<T>, meta::RemoveReference<U>,
                                                                       meta::RemoveReference<U>>::Type&>())> {};

    template<concepts::RValueReference T, concepts::RValueReference U>
    requires(requires { typename SimpleCommonReference<T&, U&>::Type; } &&
             concepts::ImplicitlyConvertibleTo<T, typename SimpleCommonReference<T&, U&>::Type> &&
             concepts::ImplicitlyConvertibleTo<U, typename SimpleCommonReference<T&, U&>::Type>)
    struct SimpleCommonReference<T, U> : TypeConstant<typename SimpleCommonReference<T&, U&>::Type> {};

    template<concepts::LValueReference T, concepts::RValueReference U>
    requires(requires { typename SimpleCommonReference<T, meta::RemoveReference<U> const&>::Type; } &&
             concepts::ImplicitlyConvertibleTo<U &&, typename SimpleCommonReference<T, meta::RemoveReference<U> const&>::Type>)
    struct SimpleCommonReference<T, U> : TypeConstant<typename SimpleCommonReference<T, meta::RemoveReference<U> const&>::Type> {};

    template<concepts::RValueReference T, concepts::LValueReference U>
    struct SimpleCommonReference<T, U> : SimpleCommonReference<U, T> {};

    template<typename T, typename U>
    concept HasSimpleCommonReference = requires { typename SimpleCommonReference<T, U>::Type; };

    template<typename T, typename U>
    concept HasCustomCommonReference =
        requires { typename CustomCommonReference<T, U, ProjectQualifiers<T>::template Type, ProjectQualifiers<U>::template Type>::Type; };

    template<typename T, typename U>
    concept HasValueCommonReference = requires { false ? __get_value<T>() : __get_value<U>(); };

    template<typename T, typename U>
    concept HasCommonTypeCommonReference = requires { typename CommonType<T, U>; };

    template<typename... Types>
    struct CommonReferenceHelper {};

    template<typename T>
    struct CommonReferenceHelper<T> : TypeConstant<T> {};

    template<typename T, typename U>
    requires(HasSimpleCommonReference<T, U>)
    struct CommonReferenceHelper<T, U> : TypeConstant<typename SimpleCommonReference<T, U>::Type> {};

    template<typename T, typename U>
    requires(!HasSimpleCommonReference<T, U> && HasCustomCommonReference<T, U>)
    struct CommonReferenceHelper<T, U>
        : TypeConstant<
              typename CustomCommonReference<T, U, ProjectQualifiers<T>::template Type, ProjectQualifiers<U>::template Type>::Type> {};

    template<typename T, typename U>
    requires(!HasSimpleCommonReference<T, U> && !HasCustomCommonReference<T, U> && HasValueCommonReference<T, U>)
    struct CommonReferenceHelper<T, U> : TypeConstant<decltype(false ? __get_value<T>() : __get_value<U>())> {};

    template<typename T, typename U>
    requires(!HasSimpleCommonReference<T, U> && !HasCustomCommonReference<T, U> && !HasValueCommonReference<T, U> &&
             HasCommonTypeCommonReference<T, U>)
    struct CommonReferenceHelper<T, U> : TypeConstant<CommonType<T, U>> {};

    template<typename T, typename U, typename W, typename... Rest>
    requires(requires { typename CommonReferenceHelper<T, U>::Type; })
    struct CommonReferenceHelper<T, U, W, Rest...> : CommonReferenceHelper<typename CommonTypeHelper<T, U>::Type, W, Rest...> {};
}

template<typename... Types>
requires(requires { typename detail::CommonReferenceHelper<Types...>::Type; })
using CommonReference = detail::CommonReferenceHelper<Types...>::Type;
}