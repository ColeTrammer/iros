#pragma once

namespace LIIM::TypeList {
template<typename ToFind, size_t index, typename... Types>
struct IndexImpl;
template<typename ToFind, size_t index, typename Type, typename... Types>
struct IndexImpl<ToFind, index, Type, Types...> {
    enum { value = IsSame<ToFind, Type>::value ? index : static_cast<size_t>(IndexImpl<ToFind, index + 1, Types...>::value) };
};
template<typename ToFind, size_t index>
struct IndexImpl<ToFind, index> {
    enum { value = -1 };
};

template<typename ToFind, typename... Types>
struct Index {
    enum { value = IndexImpl<ToFind, 0, Types...>::value };
};

template<typename... Types>
struct Size;
template<typename T, typename... Types>
struct Size<T, Types...> {
    enum { value = sizeof(T) >= Size<Types...>::value ? sizeof(T) : static_cast<size_t>(Size<Types...>::value) };
};
template<typename T>
struct Size<T> {
    enum { value = sizeof(T) };
};

template<typename T, bool already_found, typename... Types>
struct IsValidImpl;
template<typename T, bool already_found, typename T1, typename... Types>
struct IsValidImpl<T, already_found, T1, Types...> {
    enum {
        value = (IsConvertible<T, T1>::value) ? (already_found ? false : static_cast<bool>(IsValidImpl<T, true, Types...>::value))
                                              : static_cast<bool>(IsValidImpl<T, already_found, Types...>::value)
    };
    typedef typename Conditional<IsConvertible<T, T1>::value, T1, typename IsValidImpl<T, already_found, Types...>::type>::type type;
};
template<typename T, bool already_found, typename T1>
struct IsValidImpl<T, already_found, T1> {
    enum { value = already_found ^ IsConvertible<T, T1>::value };
    typedef T1 type;
};

template<typename T, typename... Types>
struct IsValid {
    enum { value = static_cast<int>(Index<T, Types...>::value) == -1 ? static_cast<bool>(IsValidImpl<T, false, Types...>::value) : true };
    typedef typename Conditional<static_cast<int>(Index<T, Types...>::value) != -1, T, typename IsValidImpl<T, false, Types...>::type>::type
        type;
};

template<typename... Types>
struct First;
template<typename T, typename... Types>
struct First<T, Types...> {
    typedef T type;
};
template<typename T>
struct First<T> {
    typedef T type;
};

template<size_t index, typename... Types>
struct TypeAtIndex;
template<size_t index, typename T, typename... Types>
struct TypeAtIndex<index, T, Types...> {
    typedef typename Conditional<index == 0, T, typename TypeAtIndex<index - 1, Types...>::type>::type type;
};
template<size_t index>
struct TypeAtIndex<index> {
    typedef void type;
};

template<typename... Types>
struct Count;
template<typename T, typename... Types>
struct Count<T, Types...> {
    enum { value = 1 + Count<Types...>::value };
};
template<>
struct Count<> {
    enum { value = 0 };
};
}
