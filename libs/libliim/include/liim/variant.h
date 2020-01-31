#pragma once

#include <assert.h>
#include <liim/utilities.h>
#include <stddef.h>

namespace LIIM {

using LIIM::Conditional;
using LIIM::FalseType;
using LIIM::IsConvertible;
using LIIM::TrueType;

namespace TypeList {
    template<typename ToFind, int index, typename... Types> struct IndexImpl;
    template<typename ToFind, int index, typename Type, typename... Types> struct IndexImpl<ToFind, index, Type, Types...> {
        enum { value = IsSame<ToFind, Type>::value ? index : IndexImpl<ToFind, index + 1, Types...>::value };
    };
    template<typename ToFind, int index> struct IndexImpl<ToFind, index> {
        enum { value = -1 };
    };

    template<typename ToFind, typename... Types> struct Index {
        enum { value = IndexImpl<ToFind, 0, Types...>::value };
    };

    template<typename... Types> struct Size;
    template<typename T, typename... Types> struct Size<T, Types...> {
        enum { value = sizeof(T) >= Size<Types...>::value ? sizeof(T) : Size<Types...>::value };
    };
    template<typename T> struct Size<T> {
        enum { value = sizeof(T) };
    };

    template<typename T, bool already_found, typename... Types> struct IsValidImpl;
    template<typename T, bool already_found, typename T1, typename... Types> struct IsValidImpl<T, already_found, T1, Types...> {
        enum {
            value = IsConvertible<T, T1>::value ? (already_found ? false : !IsValidImpl<T, true, Types...>::value)
                                                : IsValidImpl<T, already_found, Types...>::value
        };
    };
    template<typename T, bool already_found, typename T1> struct IsValidImpl<T, already_found, T1> {
        enum { value = already_found ^ IsConvertible<T, T1>::value };
    };

    template<typename T, typename... Types> struct IsValid : IsValidImpl<T, false, Types...> {};

    template<typename... Types> struct First;
    template<typename T, typename... Types> struct First<T, Types...> { typedef T type; };
    template<typename T> struct First<T> { typedef T type; };

    template<typename From, typename... Types> struct FirstConvertible;
    template<typename From, typename To, typename... Types> struct FirstConvertible<From, To, Types...> {
        typedef typename Conditional<IsConvertible<From, To>::value, To, typename FirstConvertible<From, Types...>::type>::type type;
    };
    template<typename From, typename To> struct FirstConvertible<From, To> {
        static_assert(IsConvertible<From, To>::value);
        typedef To type;
    };

    template<int index, typename... Types> struct TypeAtIndex;
    template<int index, typename T, typename... Types> struct TypeAtIndex<index, T, Types...> {
        typedef typename Conditional<index == 0, T, typename TypeAtIndex<index - 1, Types...>::type>::type type;
    };
    template<int index> struct TypeAtIndex<index> { typedef void type; };

    template<typename... Types> struct Count;
    template<typename T, typename... Types> struct Count<T, Types...> {
        enum { value = 1 + Count<Types...>::value };
    };
    template<> struct Count<> {
        enum { value = 0 };
    };

    template<typename Visitor, typename... Types> struct FunctionTable;
}

template<typename... Types> struct Variant;

template<typename... Types> struct Variant {
public:
    Variant() {
        using FirstType = typename TypeList::First<Types...>::type;
        new (&m_value_storage[0]) FirstType();
    }

    template<typename T> Variant(const T& other, typename EnableIf<TypeList::IsValid<T, Types...>::value>::type* = 0) {
        using RealType = typename TypeList::FirstConvertible<T, Types...>::type;
        new (&m_value_storage[0]) RealType(other);
    }

    template<typename T> Variant(T&& other, typename EnableIf<TypeList::IsValid<T, Types...>::value>::type* = 0) {
        using RealType = typename TypeList::FirstConvertible<T, Types...>::type;
        new (&m_value_storage[0]) RealType(move(other));
    }

    template<typename R, typename Visitor> constexpr R visit(Visitor&& visitor) {
        return TypeList::FunctionTable<Visitor, Types...>::table[m_value_index](m_value_storage);
    }

    template<int index> constexpr typename TypeList::TypeAtIndex<index, Types...>::type& get() {
        assert(m_value_index == index);
        using RealType = typename TypeList::TypeAtIndex<index, Types...>::type;
        return *reinterpret_cast<RealType*>(&m_value_storage[0]);
    }

    template<typename T> constexpr T& as() {
        constexpr int index = TypeList::Index<T, Types...>::value;
        return this->get<index>();
    }

    template<int index> constexpr bool is() const { return m_value_index == index; }

    template<typename T> constexpr bool is() const {
        constexpr int index = TypeList::Index<T, Types...>::value;
        return this->is<index>();
    }

    constexpr int index() const { return m_value_index; }

private:
    int m_value_index { 0 };
    unsigned char m_value_storage[TypeList::Size<Types...>::value];
};

}

using LIIM::Variant;