#pragma once

#ifndef __is_libc
#include <new>
#else
#include <stddef.h>

inline void* operator new(size_t, void* p) {
    return p;
}
inline void* operator new[](size_t, void* p) {

    return p;
}

inline void operator delete(void*, void*) {};
inline void operator delete[](void*, void*) {};
#endif /* __is_libc */

extern "C" {
void* malloc(size_t n);
void* realloc(void* p, size_t n);
void free(void* p);
void* calloc(size_t n, size_t sz);
}

namespace std {
typedef decltype(nullptr) nullptr_t;
typedef __SIZE_TYPE__ size_t;
}

namespace LIIM {

struct TrueType {
    enum { value = true };
};

struct FalseType {
    enum { value = false };
};

template<typename T> struct TypeIdentity { typedef T type; };

template<bool B, typename T = void> struct EnableIf {};
template<typename T> struct EnableIf<true, T> { typedef T type; };

template<typename A, typename B> struct IsSame : FalseType {};

template<typename T> struct IsSame<T, T> : TrueType {};

template<typename T> struct IsArray : FalseType {};
template<typename T> struct IsArray<T[]> : TrueType {};
template<typename T, std::size_t N> struct IsArray<T[N]> : TrueType {};

template<typename T> struct IsPointer : FalseType {};
template<typename T> struct IsPointer<T*> : TrueType {};

template<typename T> struct IsReference : FalseType {};
template<typename T> struct IsReference<T&> : TrueType {};
template<typename T> struct IsReference<T&&> : TrueType {};

template<typename T> struct IsConst : FalseType {};
template<typename T> struct IsConst<const T> : TrueType {};

template<typename T> struct IsFunction {
    enum { value = !IsConst<const T>::value && !IsReference<T>::value };
};

template<class T> struct IsRValueReference : FalseType {};
template<class T> struct IsRValueReference<T&&> : TrueType {};

template<typename T> struct RemovePointer { typedef T type; };
template<typename T> struct RemovePointer<T*> { typedef T type; };
template<typename T> struct RemovePointer<T* const> { typedef T type; };
template<typename T> struct RemovePointer<T* volatile> { typedef T type; };
template<typename T> struct RemovePointer<T* const volatile> { typedef T type; };

template<typename T> struct RemoveReference { typedef T type; };
template<typename T> struct RemoveReference<T&> { typedef T type; };
template<typename T> struct RemoveReference<T&&> { typedef T type; };

template<typename T> struct RemoveExtent { typedef T type; };
template<typename T> struct RemoveExtent<T[]> { typedef T type; };
template<typename T, std::size_t N> struct RemoveExtent<T[N]> { typedef T type; };

template<typename T> struct RemoveConst { typedef T type; };
template<typename T> struct RemoveConst<const T> { typedef T type; };

template<typename T> struct RemoveVolatile { typedef T type; };
template<typename T> struct RemoveVolatile<volatile T> { typedef T type; };

template<typename T> struct RemoveCV { typedef typename RemoveConst<typename RemoveVolatile<T>::type>::type type; };

namespace details {
    template<typename T> auto try_add_pointer(int) -> TypeIdentity<typename RemoveReference<T>::type*>;
    template<typename T> auto try_add_pointer(...) -> TypeIdentity<T>;
}

template<typename T> struct AddPointer : decltype(details::try_add_pointer<T>(0)) {};

template<bool Z, typename A, typename B> struct Conditional { typedef B type; };
template<typename A, typename B> struct Conditional<true, A, B> { typedef A type; };

template<typename T> T& declval() {
    return 0;
}

namespace details {
    template<typename> using TrueTypeFor = TrueType;

    template<typename T> auto test_returnable(int) -> TrueTypeFor<T()>;
    template<typename T> auto test_returnable(...) -> FalseType;

    template<typename From, typename To>
    auto test_nonvoid_convertible(int) -> TrueTypeFor<decltype(declval<void (&)(To)>()(declval<From>()))>;
    template<typename From, typename To> auto test_nonvoid_convertible(...) -> FalseType;
}

template<typename From, typename To> struct IsConvertible {
    enum { value = (decltype(details::test_returnable<To>(0))::value&& decltype(details::test_nonvoid_convertible<From, To>(0))::value) };
};

template<typename T> struct Decay {
private:
    typedef typename RemoveReference<T>::type U;

public:
    typedef typename Conditional<
        IsArray<U>::value, typename RemoveExtent<U>::type*,
        typename Conditional<IsFunction<U>::value, typename AddPointer<U>::type, typename RemoveCV<U>::type>::type>::type type;
};

template<typename T> using decay_t = typename Decay<T>::type;

template<typename T> inline constexpr T&& forward(typename TypeIdentity<T>::type& param) {
    return static_cast<T&&>(param);
}

template<typename T> inline typename RemoveReference<T>::type&& move(T&& arg) {
    return static_cast<typename RemoveReference<T>::type&&>(arg);
}

template<typename T> void swap(T& a, T& b) {
    T temp(a);
    a = b;
    b = temp;
}

}

using LIIM::move;