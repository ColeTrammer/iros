#pragma once

#include <assert.h>
#include <liim/tuple.h>
#include <liim/utilities.h>
#include <stddef.h>
#include <utility>

namespace LIIM {

using LIIM::Conditional;
using LIIM::FalseType;
using LIIM::IsConvertible;
using LIIM::TrueType;

namespace TypeList {
    template<typename ToFind, size_t index, typename... Types> struct IndexImpl;
    template<typename ToFind, size_t index, typename Type, typename... Types> struct IndexImpl<ToFind, index, Type, Types...> {
        enum { value = IsSame<ToFind, Type>::value ? index : IndexImpl<ToFind, index + 1, Types...>::value };
    };
    template<typename ToFind, size_t index> struct IndexImpl<ToFind, index> {
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
            value = (IsConvertible<T, T1>::value) ? (already_found ? false : !IsValidImpl<T, true, Types...>::value)
                                                  : IsValidImpl<T, already_found, Types...>::value
        };
        typedef T type;
    };
    template<typename T, bool already_found, typename T1> struct IsValidImpl<T, already_found, T1> {
        enum { value = already_found ^ IsConvertible<T, T1>::value };
        typedef T type;
    };

    template<typename T, typename... Types> struct IsValid {
        enum { value = Index<T, Types...>::value != -1 ? IsValidImpl<T, false, Types...>::value : true };
        typedef typename Conditional<Index<T, Types...>::value != -1, T, typename IsValidImpl<T, false, Types...>::type>::type type;
    };

    template<typename... Types> struct First;
    template<typename T, typename... Types> struct First<T, Types...> { typedef T type; };
    template<typename T> struct First<T> { typedef T type; };

    template<size_t index, typename... Types> struct TypeAtIndex;
    template<size_t index, typename T, typename... Types> struct TypeAtIndex<index, T, Types...> {
        typedef typename Conditional<index == 0, T, typename TypeAtIndex<index - 1, Types...>::type>::type type;
    };
    template<size_t index> struct TypeAtIndex<index> { typedef void type; };

    template<typename... Types> struct Count;
    template<typename T, typename... Types> struct Count<T, Types...> {
        enum { value = 1 + Count<Types...>::value };
    };
    template<> struct Count<> {
        enum { value = 0 };
    };

    template<typename T> struct Box {
        T& get_as_value() { return *reinterpret_cast<T*>(&m_storage[0]); }
        unsigned char m_storage[sizeof(T)];
    };

    template<typename... Types> union VariadicUnion;
    template<typename First, typename... Last> union VariadicUnion<First, Last...> {
        Box<First> m_first;
        VariadicUnion<Last...> m_last;
    };
    template<> union VariadicUnion<> {};
}

template<typename Visitor, typename... Variants> inline constexpr decltype(auto) visit(Visitor&& vis, Variants&&...);
template<typename Visitor, typename Variant> inline constexpr decltype(auto) variant_get(Visitor&& vis);

template<typename... Types> class Variant {
public:
    Variant() {
        using FirstType = typename TypeList::First<Types...>::type;
        new (&m_value_storage) FirstType();
    }

    template<typename T> Variant(const T& other, typename EnableIf<TypeList::IsValid<T, Types...>::value>::type* = 0) {
        using RealType = typename TypeList::IsValid<T, Types...>::type;
        constexpr size_t index = TypeList::Index<RealType, Types...>::value;
        new (&m_value_storage) RealType(other);
        m_value_index = index;
    }

    template<typename T> Variant(T&& other, typename EnableIf<TypeList::IsValid<T, Types...>::value>::type* = 0) {
        using RealType = typename TypeList::IsValid<T, Types...>::type;
        constexpr size_t index = TypeList::Index<RealType, Types...>::value;
        new (&m_value_storage) RealType(move(other));
        m_value_index = index;
    }

    ~Variant() { destroy(); }

    Variant& operator=(const Variant& other) {
        if (this != &other) {
            Variant temp(other);
            swap(temp);
        }
        return *this;
    }
    Variant& operator=(Variant&& other) {
        if (this != &other) {
            Variant temp(LIIM::move(other));
            swap(temp);
        }
        return *this;
    }
    template<typename T, typename = typename EnableIf<TypeList::IsValid<T, Types...>::value>::type> Variant& operator=(const T& value) {
        Variant temp(value);
        swap(temp);
        return *this;
    }
    template<typename T, typename = typename EnableIf<TypeList::IsValid<T, Types...>::value>::type> Variant& operator=(T&& value) {
        Variant temp(LIIM::move(value));
        swap(temp);
        return *this;
    }

    template<typename T> constexpr T* get_if() {
        constexpr size_t index = TypeList::Index<T, Types...>::value;
        if (m_value_index != index) {
            return nullptr;
        }
        return &this->get<index>();
    }
    template<typename T> constexpr const T* get_if() {
        constexpr size_t index = TypeList::Index<T, Types...>::value;
        if (m_value_index != index) {
            return nullptr;
        }
        return &this->get<index>();
    }

    template<size_t index> constexpr typename TypeList::TypeAtIndex<index, Types...>::type& get() {
        assert(m_value_index == index);
        using RealType = typename TypeList::TypeAtIndex<index, Types...>::type;
        return *reinterpret_cast<RealType*>(&m_value_storage);
    }
    template<size_t index> constexpr const typename TypeList::TypeAtIndex<index, Types...>::type& get() {
        return const_cast<Variant&>(*this).get<index>();
    }

    template<typename T> constexpr T& as() {
        constexpr size_t index = TypeList::Index<T, Types...>::value;
        return this->get<index>();
    }
    template<typename T> constexpr const T& as() { return const_cast<Variant&>(*this).as<T>(); }

    template<size_t index> constexpr bool is() const { return m_value_index == index; }

    template<typename T> constexpr bool is() const {
        constexpr size_t index = TypeList::Index<T, Types...>::value;
        return this->is<index>();
    }

    template<typename Visitor> constexpr decltype(auto) visit(Visitor&& vis) {
        return LIIM::visit(forward<Visitor>(vis), forward<Variant>(*this));
    }

    constexpr size_t index() const { return m_value_index; }

    void swap(Variant& other) {
        auto temp = this->visit([](auto&& a) {
            return LIIM::move(a);
        });
        auto other_value = this->visit([](auto&& a) {
            return LIIM::move(a);
        });
        using OtherType = decay_t<decltype(other_value)>;
        destroy();
        new (&m_value_storage) OtherType(LIIM::move(other_value));

        using TempType = decay_t<decltype(temp)>;
        other.destroy();
        new (&other.m_value_storage) TempType(LIIM::move(temp));
        LIIM::swap(this->m_value_index, other.m_value_index);
    }

    static constexpr size_t num_variants() { return TypeList::Count<Types...>::value; }

    bool operator!=(const Variant& other) const {
        return LIIM::visit(
            [&](auto&& a, auto&& b) -> bool {
                return a != b;
            },
            forward<const Variant>(*this), forward<const Variant>(other));
    }
    bool operator==(const Variant& other) const {
        return LIIM::visit(
            [&](auto&& a, auto&& b) -> bool {
                return a == b;
            },
            forward<const Variant>(*this), forward<const Variant>(other));
    }
    bool operator<=(const Variant& other) const {
        return LIIM::visit(
            [&](auto&& a, auto&& b) -> bool {
                return a <= b;
            },
            forward<const Variant>(*this), forward<const Variant>(other));
    }
    bool operator>=(const Variant& other) const {
        return LIIM::visit(
            [&](auto&& a, auto&& b) -> bool {
                return a >= b;
            },
            forward<const Variant>(*this), forward<const Variant>(other));
    }
    bool operator<(const Variant& other) const {
        return LIIM::visit(
            [&](auto&& a, auto&& b) -> bool {
                return a < b;
            },
            forward<const Variant>(*this), forward<const Variant>(other));
    }
    bool operator>(const Variant& other) const {
        return LIIM::visit(
            [&](auto&& a, auto&& b) -> bool {
                return a > b;
            },
            forward<const Variant>(*this), forward<const Variant>(other));
    }

private:
    template<typename Visitor, typename... Variants> friend inline constexpr decltype(auto) visit(Visitor&&, Variants&&...);
    template<size_t index, typename Variant> friend inline constexpr decltype(auto) variant_get(Variant&&);

    void destroy() {
        this->visit([this](auto&& val) {
            using T = decay_t<decltype(val)>;
            forward<T>(val).~T();
        });
    }

    TypeList::VariadicUnion<Types...> m_value_storage;
    size_t m_value_index { 0 };
};

template<typename... Types> void swap(Variant<Types...>& a, Variant<Types...>& b) {
    a.swap(b);
}

template<size_t index, typename Union> inline constexpr decltype(auto) variant_get(std::in_place_index_t<0>, Union&& u);

template<typename Union> inline constexpr decltype(auto) variant_get(std::in_place_index_t<0>, Union&& u) {
    return u.m_first.get_as_value();
}

template<size_t index, typename Union> inline constexpr decltype(auto) variant_get(std::in_place_index_t<index>, Union&& u) {
    return LIIM::variant_get(std::in_place_index<index - 1>, forward<Union>(u).m_last);
}

template<size_t index, typename Variant> inline constexpr decltype(auto) variant_get(Variant&& v) {
    return LIIM::variant_get(std::in_place_index<index>, forward<Variant>(v).m_value_storage);
}

template<typename T, size_t... place> struct Array;
template<typename T> struct Array<T> {
    constexpr const T& access() const { return m_value; }

    T m_value;
};
template<typename R, typename Visitor, typename... Variants, size_t head, size_t... tail>
struct Array<R (*)(Visitor, Variants...), head, tail...> {
    static constexpr size_t index_in_array = sizeof...(Variants) - sizeof...(tail) - 1;

    using FunctionType = R (*)(Visitor, Variants...);

    template<typename... Args> constexpr const FunctionType& access(size_t first_index, Args... rest) const {
        return m_array[first_index].access(rest...);
    }

    Array<FunctionType, tail...> m_array[head];
};

template<typename ArrayType, typename VariantTuple, typename IndexSequence> struct ArrayBuilderImpl;
template<typename R, typename Visitor, size_t... dimensions, typename... Variants, size_t... indicies>
struct ArrayBuilderImpl<Array<R (*)(Visitor, Variants...), dimensions...>, Tuple<Variants...>, IndexSequence<indicies...>> {
    static constexpr size_t variant_index = sizeof...(indicies);
    using VariantType = typename RemoveReference<typename TypeList::TypeAtIndex<variant_index, Variants...>::type>::type;
    using ArrayType = Array<R (*)(Visitor, Variants...), dimensions...>;

    static constexpr ArrayType apply() {
        ArrayType table {};
        apply_all(table, LIIM::make_index_sequence<VariantType::num_variants()>());
        return table;
    }

    template<size_t... variant_indicies> static constexpr void apply_all(ArrayType& table, IndexSequence<variant_indicies...>) {
        (apply_one<variant_indicies>(table, table.m_array[variant_indicies]), ...);
    }

    template<size_t index, typename T> static constexpr void apply_one(ArrayType& array, T& element) {
        element = ArrayBuilderImpl<typename RemoveReference<decltype(element)>::type, Tuple<Variants...>,
                                   IndexSequence<indicies..., index>>::apply();
    }
};

template<typename R, typename Visitor, typename... Variants, size_t... indices>
struct ArrayBuilderImpl<Array<R (*)(Visitor, Variants...)>, Tuple<Variants...>, IndexSequence<indices...>> {
    using ArrayType = Array<R (*)(Visitor, Variants...)>;

    template<size_t index, typename Variant> static constexpr decltype(auto) element_in_variant_by_index(Variant&& var) {
        return variant_get<index>(forward<Variant>(var));
    }

    static constexpr decltype(auto) visit_invoke_impl(Visitor&& visitor, Variants... vars) {
        if constexpr (IsVoid<R>::value) {
            return (void) LIIM::invoke(forward<Visitor>(visitor), element_in_variant_by_index<indices>(forward<Variants>(vars))...);
        } else
            return LIIM::invoke(forward<Visitor>(visitor), element_in_variant_by_index<indices>(forward<Variants>(vars))...);
    }

    static constexpr decltype(auto) do_visit_invoke(Visitor&& visitor, Variants... vars) {
        return visit_invoke_impl(forward<Visitor>(visitor), forward<Variants>(vars)...);
    }

    static constexpr decltype(auto) visit_invoke(Visitor&& visitor, Variants... vars) {
        return do_visit_invoke(forward<Visitor>(visitor), forward<Variants>(vars)...);
    }

    static constexpr auto apply() { return ArrayType { &visit_invoke }; }
};

template<typename R, typename Visitor, typename... Variants> struct ArrayBuilder {
    using ArrayType = Array<R (*)(Visitor, Variants...), (RemoveReference<Variants>::type::num_variants())...>;

    static constexpr ArrayType table = ArrayBuilderImpl<ArrayType, Tuple<Variants...>, IndexSequence<>>::apply();
};

template<typename Visitor, typename... Variants> inline constexpr decltype(auto) visit(Visitor&& vis, Variants&&... vs) {
    using R = typename InvokeResult<Visitor, decltype(variant_get<0>(LIIM::declval<Variants>()))...>::type;

    constexpr auto& table = ArrayBuilder<R, Visitor&&, Variants&&...>::table;

    auto func_ptr = table.access(vs.index()...);
    return (*func_ptr)(forward<Visitor>(vis), forward<Variants>(vs)...);
}

struct Monostate {};

}

using LIIM::Monostate;
using LIIM::Variant;
using LIIM::visit;