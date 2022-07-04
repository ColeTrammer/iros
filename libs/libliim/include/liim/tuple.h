#pragma once

#include <liim/type_list.h>

namespace LIIM {
namespace Detail {
    template<typename... Types>
    class TupleImpl;

    template<typename T, typename... Rest>
    class TupleImpl<T, Rest...> {
    public:
        constexpr TupleImpl() = default;
        constexpr TupleImpl(T&& value, Rest&&... rest) : m_value(forward<T>(value)), m_rest(forward<Rest>(rest)...) {}
        constexpr TupleImpl(const TupleImpl&) = default;
        constexpr TupleImpl(TupleImpl&&) = default;

        constexpr ~TupleImpl() = default;

        constexpr TupleImpl& operator=(const TupleImpl&) = default;
        constexpr TupleImpl& operator=(TupleImpl&&) = default;

        template<size_t index>
        static constexpr typename TypeList::TypeAtIndex<index, T, Rest...>::type& get(TupleImpl& impl) {
            static_assert(index < TypeList::Count<T, Rest...>::value, "Tuple get index is too high");
            if constexpr (index == 0) {
                return impl.m_value;
            } else {
                return TupleImpl<Rest...>::template get<index - 1>(impl.m_rest);
            }
        }

        template<size_t index>
        static constexpr typename TypeList::TypeAtIndex<index, T, Rest...>::type&& get(TupleImpl&& impl) {
            static_assert(index < TypeList::Count<T, Rest...>::value, "Tuple get index is too high");
            if constexpr (index == 0) {
                return forward<T&&>(impl.m_value);
            } else {
                return TupleImpl<Rest...>::template get<index - 1>(move(impl).m_rest);
            }
        }

        template<size_t index>
        static constexpr const typename TypeList::TypeAtIndex<index, T, Rest...>::type& get(const TupleImpl& impl) {
            static_assert(index < TypeList::Count<T, Rest...>::value, "Tuple get index is too high");
            if constexpr (index == 0) {
                return impl.m_value;
            } else {
                return TupleImpl<Rest...>::template get<index - 1>(impl.m_rest);
            }
        }

        template<size_t index>
        static constexpr const typename TypeList::TypeAtIndex<index, T, Rest...>::type&& get(const TupleImpl&& impl) {
            static_assert(index < TypeList::Count<T, Rest...>::value, "Tuple get index is too high");
            if constexpr (index == 0) {
                return forward<const T&&>(impl.m_value);
            } else {
                return TupleImpl<Rest...>::template get<index - 1>(move(impl).m_rest);
            }
        }

        template<typename... Types>
        friend class TupleImpl;

    private:
        T m_value {};
        TupleImpl<Rest...> m_rest;
    };

    template<>
    class TupleImpl<> {};
}

template<typename... Types>
class Tuple {
public:
    static constexpr size_t size() { return TypeList::Count<Types...>::value; }

    constexpr Tuple() = default;
    constexpr Tuple(Types&&... types) : m_impl(forward<Types>(types)...) {}
    constexpr Tuple(const Tuple&) = default;
    constexpr Tuple(Tuple&&) = default;

    constexpr ~Tuple() = default;

    constexpr Tuple& operator=(const Tuple&) = default;
    constexpr Tuple& operator=(Tuple&&) = default;

    template<size_t index>
    constexpr decltype(auto) get() & {
        return Detail::TupleImpl<Types...>::template get<index>(m_impl);
    }

    template<size_t index>
    constexpr decltype(auto) get() const& {
        return Detail::TupleImpl<Types...>::template get<index>(m_impl);
    }

    template<size_t index>
    constexpr decltype(auto) get() && {
        return Detail::TupleImpl<Types...>::template get<index>(move(m_impl));
    }

    template<size_t index>
    constexpr decltype(auto) get() const&& {
        return Detail::TupleImpl<Types...>::template get<index>(move(m_impl));
    }

private:
    Detail::TupleImpl<Types...> m_impl;
};

template<class... Types>
Tuple(Types...) -> Tuple<Types...>;

template<typename... Args>
constexpr Tuple<decay_t<Args>...> make_tuple(Args&&... args) {
    return Tuple<decay_t<Args>...>(forward<Args>(args)...);
}

template<typename... Args>
constexpr Tuple<Args&&...> forward_as_tuple(Args&&... args) {
    return Tuple<Args&&...>(forward<Args>(args)...);
}

template<typename C, typename Tuple>
constexpr auto tuple_map(Tuple&& tuple, C&& mapper) {
    auto helper = [&]<size_t... indices>(IndexSequence<indices...>)->decltype(auto) {
        using ReturnTuple = LIIM::Tuple<decltype(mapper(forward<Tuple>(tuple).template get<indices>()))...>;
        return ReturnTuple(mapper(forward<Tuple>(tuple).template get<indices>())...);
    };
    return helper(make_index_sequence<std::tuple_size<decay_t<Tuple>>::value>());
}

template<typename C, typename Tuple>
constexpr decltype(auto) tuple_apply(C&& callable, Tuple&& tuple) {
    auto helper = [&]<size_t... indices>(IndexSequence<indices...>)->decltype(auto) {
        return forward<C>(callable)(forward<Tuple>(tuple).template get<indices>()...);
    };
    return helper(make_index_sequence<std::tuple_size<decay_t<Tuple>>::value>());
}

template<typename T, typename Tuple>
constexpr T make_from_tuple(Tuple&& tuple) {
    auto helper = [&]<size_t... indices>(IndexSequence<indices...>)->T {
        return T(forward<Tuple>(tuple).template get<indices>()...);
    };
    return helper(make_index_sequence<std::tuple_size<decay_t<Tuple>>::value>());
}
};

namespace std {
template<typename... Types>
struct tuple_size<LIIM::Tuple<Types...>> {
    static constexpr size_t value = LIIM::TypeList::Count<Types...>::value;
};

template<size_t index, typename... Types>
struct tuple_element<index, LIIM::Tuple<Types...>> {
    using type = LIIM::TypeList::TypeAtIndex<index, Types...>::type;
};
}

using LIIM::forward_as_tuple;
using LIIM::make_from_tuple;
using LIIM::make_tuple;
using LIIM::Tuple;
using LIIM::tuple_apply;
using LIIM::tuple_map;
