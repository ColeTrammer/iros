#pragma once

#include <assert.h>
#include <liim/utility.h>
#include <stddef.h>

struct TrueType {
    enum { value = true };
};

struct FalseType {
    enum { value = false };
};

template<bool B, typename T = void> struct EnableIf {};
template<typename T> struct EnableIf<true, T> { typedef T type; };

template<typename A, typename B> struct IsSame : FalseType {};

template<typename T> struct IsSame<T, T> : TrueType {};

static_assert(!IsSame<int, double>::value);
static_assert(IsSame<int, int>::value);

template<typename ToFind, int index, typename... Types> struct TypeListIndexImpl;

template<typename ToFind, int index, typename Type, typename... Types> struct TypeListIndexImpl<ToFind, index, Type, Types...> {
    enum { value = IsSame<ToFind, Type>::value ? index : TypeListIndexImpl<ToFind, index + 1, Types...>::value };
};

template<typename ToFind, int index> struct TypeListIndexImpl<ToFind, index> {
    enum { value = -1 };
};

template<typename ToFind, typename... Types> struct TypeListIndex {
    enum { value = TypeListIndexImpl<ToFind, 0, Types...>::value };
};

static_assert(TypeListIndex<int, double, float, int>::value == 2);
static_assert(TypeListIndex<long, double, float, int>::value == -1);

template<typename... Types> struct TypeListSize;

template<typename T, typename... Types> struct TypeListSize<T, Types...> {
    enum { value = sizeof(T) >= TypeListSize<Types...>::value ? sizeof(T) : TypeListSize<Types...>::value };
};

template<typename T> struct TypeListSize<T> {
    enum { value = sizeof(T) };
};

static_assert(TypeListSize<int, unsigned char, unsigned long>::value == sizeof(unsigned long));

template<typename> using TrueTypeFor = TrueType;

template<typename T> auto test_returnable(int) -> TrueTypeFor<T()>;
template<typename T> auto test_returnable(...) -> FalseType;

template<typename T> T& declval() {
    return 0;
}

template<typename From, typename To> auto test_nonvoid_convertible(int) -> TrueTypeFor<decltype(declval<void (&)(To)>()(declval<From>()))>;
template<typename From, typename To> auto test_nonvoid_convertible(...) -> FalseType;

template<typename From, typename To> struct IsConvertible {
    enum { value = (decltype(test_returnable<To>(0))::value&& decltype(test_nonvoid_convertible<From, To>(0))::value) };
};

static_assert(IsConvertible<std::nullptr_t, int*>::value);

template<typename T, bool already_found, typename... Types> struct TypeListIsValidImpl;

template<typename T, bool already_found, typename T1, typename... Types> struct TypeListIsValidImpl<T, already_found, T1, Types...> {
    enum {
        value = IsConvertible<T, T1>::value ? (already_found ? false : !TypeListIsValidImpl<T, true, Types...>::value)
                                            : TypeListIsValidImpl<T, already_found, Types...>::value
    };
};

template<typename T, bool already_found, typename T1> struct TypeListIsValidImpl<T, already_found, T1> {
    enum { value = already_found ^ IsConvertible<T, T1>::value };
};

template<typename T, typename... Types> struct TypeListIsValid : TypeListIsValidImpl<T, false, Types...> {};

template<typename... Types> struct TypeListFirst;
template<typename T, typename... Types> struct TypeListFirst<T, Types...> { typedef T type; };
template<typename T> struct TypeListFirst<T> { typedef T type; };

template<bool, typename T, typename U> struct TypeIf;
template<typename T, typename U> struct TypeIf<true, T, U> { typedef T type; };
template<typename T, typename U> struct TypeIf<false, T, U> { typedef U type; };

template<typename From, typename... Types> struct TypeListFirstConvertible;
template<typename From, typename To, typename... Types> struct TypeListFirstConvertible<From, To, Types...> {
    typedef TypeIf<IsConvertible<From, To>::value, To, typename TypeListFirstConvertible<From, Types...>::type>::type type;
};
template<typename From, typename To> struct TypeListFirstConvertible<From, To> {
    static_assert(IsConvertible<From, To>::value);
    typedef To type;
};

template<int index, typename... Types> struct TypeListTypeAtIndex;
template<int index, typename T, typename... Types> struct TypeListTypeAtIndex<index, T, Types...> {
    typedef TypeIf<index == 0, T, typename TypeListTypeAtIndex<index - 1, Types...>::type>::type type;
};
template<int index> struct TypeListTypeAtIndex<index> { typedef void type; };

template<typename... Types> struct TypeListCount;
template<typename T, typename... Types> struct TypeListCount<T, Types...> {
    enum { value = 1 + TypeListCount<Types...>::value };
};
template<> struct TypeListCount<> {
    enum { value = 0 };
};

template<typename Visitor, typename... Types> struct TypeListFunctionTable;

template<typename... Types> struct Variant {
public:
    Variant() {
        using FirstType = typename TypeListFirst<Types...>::type;
        new (&m_value_storage[0]) FirstType();
    }

    template<typename T> Variant(const T& other, typename EnableIf<TypeListIsValid<T, Types...>::value>::type* = 0) {
        using RealType = typename TypeListFirstConvertible<T, Types...>::type;
        new (&m_value_storage[0]) RealType(other);
    }

    template<typename T> Variant(T&& other, typename EnableIf<TypeListIsValid<T, Types...>::value>::type* = 0) {
        using RealType = typename TypeListFirstConvertible<T, Types...>::type;
        new (&m_value_storage[0]) RealType(move(other));
    }

    template<typename R, typename Visitor> constexpr R visit(Visitor&& visitor) {
        return R(TypeListFunctionTable<Visitor, Types...>::table[m_value_index](m_value_storage));
    }

    template<typename T> T& as() {
        assert(m_value_index == (TypeListIndex<T, Types...>::value));
        return *reinterpret_cast<T*>(&m_value_storage[0]);
    }

private:
    int m_value_index { 0 };
    unsigned char m_value_storage[TypeListSize<Types...>::value];
};