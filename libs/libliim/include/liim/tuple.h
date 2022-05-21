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
        constexpr ~TupleImpl() = default;

        template<size_t index, typename Impl>
        static constexpr decltype(auto) get(Impl&& impl) {
            static_assert(index < TypeList::Count<T, Rest...>::value, "Tuple get index is too high");
            if constexpr (index == 0) {
                return (forward<Impl>(impl).m_value);
            } else {
                return get<index - 1>(forward<Impl>(impl).m_rest);
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
    constexpr Tuple() = default;
    constexpr Tuple(Types&&... types) : m_impl(forward<Types>(types)...) {}
    constexpr ~Tuple() = default;

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

using LIIM::Tuple;
