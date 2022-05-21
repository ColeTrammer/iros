#pragma once

#include <assert.h>
#include <liim/fixed_array.h>
#include <liim/option.h>
#include <liim/type_list.h>
#include <liim/utilities.h>
#include <stddef.h>

namespace LIIM {

using LIIM::Conditional;
using LIIM::FalseType;
using LIIM::IsConvertible;
using LIIM::TrueType;

namespace details {
    template<typename... T>
    class Union;

    template<typename T, typename... Rest>
    class Union<T, Rest...> {
    public:
        constexpr Union() {}
        constexpr ~Union() {}

        static constexpr bool is_reference = IsLValueReference<T>::value;
        using Storage = typename Conditional<is_reference, typename RemoveReference<T>::type*, T>::type;

        template<size_t index>
        constexpr decltype(auto) access() {
            static_assert(index < TypeList::Count<T, Rest...>::value);
            if constexpr (index == 0) {
                if constexpr (is_reference) {
                    return static_cast<T>(*m_value);
                } else {
                    return static_cast<T&>(m_value);
                }
            } else {
                return (m_rest.template access<index - 1>());
            }
        }

        template<size_t index>
        constexpr decltype(auto) access() const {
            static_assert(index < TypeList::Count<T, Rest...>::value);
            if constexpr (index == 0) {
                if constexpr (is_reference) {
                    return static_cast<const T>(*m_value);
                } else {
                    return static_cast<const T&>(m_value);
                }
            } else {
                return (m_rest.template access<index - 1>());
            }
        }

        template<size_t index, typename... Args>
        constexpr void emplace(Args&&... args) {
            static_assert(index < TypeList::Count<T, Rest...>::value);
            if constexpr (index == 0) {
                if constexpr (is_reference) {
                    m_value = &T(forward<Args>(args)...);
                } else {
                    construct_at(&m_value, forward<Args>(args)...);
                }
            } else {
                m_rest.template emplace<index - 1>(forward<Args>(args)...);
            }
        }

        template<size_t index>
        constexpr void destroy() {
            static_assert(index < TypeList::Count<T, Rest...>::value);
            if constexpr (index == 0) {
                if constexpr (!is_reference) {
                    m_value.~T();
                }
            } else {
                m_rest.template destroy<index - 1>();
            }
        }

    private:
        union {
            Storage m_value;
            Union<Rest...> m_rest;
        };
    };

    template<>
    class Union<> {};

    template<typename R, typename Visitor, typename Variant>
    struct CallTable {
    public:
        using VariantType = typename RemoveReference<Variant>::type;
        static constexpr size_t N = VariantType::num_variants();

        static constexpr CallTable build() {
            auto table = CallTable {};
            table.template fill<0>();
            return table;
        }

        template<size_t index>
        constexpr void fill() {
            if constexpr (index == N) {
                return;
            } else {
                storage[index] = [](Visitor visitor, Variant variant) -> R {
                    return forward<Visitor>(visitor)(in_place_index<index>, forward<Variant>(variant));
                };
                this->fill<index + 1>();
            }
        }

        FixedArray<R (*)(Visitor, Variant), N> storage;
    };
}

template<typename Visitor, typename... Variants>
inline constexpr decltype(auto) visit(Visitor&& vis, Variants&&...);

template<typename... Types>
class Variant {
public:
    constexpr Variant() { m_storage.template emplace<0>(); }

    constexpr Variant(const Variant& other) : m_value_index(other.m_value_index) {
        other.visit_by_index([&]<size_t index>(in_place_index_t<index>, auto&& other) {
            m_storage.template emplace<index>(other.template get<index>());
        });
    }

    constexpr Variant(Variant&& other) : m_value_index(other.m_value_index) {
        other.visit_by_index([&]<size_t index>(in_place_index_t<index>, auto&& other) {
            m_storage.template emplace<index>(move(other.template get<index>()));
        });
    }

    template<typename T, typename... Args>
    constexpr Variant(in_place_type_t<T>, Args&&... args, typename EnableIf<TypeList::Index<T, Types...>::value != -1>::type* = 0) {
        constexpr size_t index = TypeList::Index<T, Types...>::value;
        static_assert(index != -1);
        m_storage.template emplace<index>(forward<Args>(args)...);
        m_value_index = index;
    }

    template<size_t index, typename... Args>
    constexpr Variant(in_place_index_t<index>, Args&&... args) {
        m_storage.template emplace<index>(forward<Args>(args)...);
        m_value_index = index;
    }

    template<typename T>
    constexpr Variant(const T& other, typename EnableIf<TypeList::IsValid<T, Types...>::value>::type* = 0) {
        using RealType = typename TypeList::IsValid<T, Types...>::type;
        constexpr size_t index = TypeList::Index<RealType, Types...>::value;
        static_assert(index != -1);
        m_storage.template emplace<index>(other);
        m_value_index = index;
    }

    template<typename T>
    constexpr Variant(T&& other, typename EnableIf<TypeList::IsValid<T, Types...>::value>::type* = 0) {
        using RealType = typename TypeList::IsValid<T, Types...>::type;
        constexpr size_t index = TypeList::Index<RealType, Types...>::value;
        static_assert(index != -1);
        m_storage.template emplace<index>(move(other));
        m_value_index = index;
    }

    constexpr ~Variant() { destroy(); }

    constexpr Variant& operator=(const Variant& other) {
        if (this != &other) {
            Variant temp(other);
            swap(temp);
        }
        return *this;
    }
    constexpr Variant& operator=(Variant&& other) {
        if (this != &other) {
            Variant temp(LIIM::move(other));
            swap(temp);
        }
        return *this;
    }
    template<typename T, typename = typename EnableIf<TypeList::IsValid<T, Types...>::value>::type>
    constexpr Variant& operator=(const T& value) {
        Variant temp(value);
        swap(temp);
        return *this;
    }
    template<typename T, typename = typename EnableIf<TypeList::IsValid<T, Types...>::value>::type>
    constexpr Variant& operator=(T&& value) {
        Variant temp(LIIM::move(value));
        swap(temp);
        return *this;
    }

    template<typename T>
    constexpr Option<T&> get_if() {
        constexpr size_t index = TypeList::Index<T, Types...>::value;
        static_assert(index != -1);
        if (m_value_index != index) {
            return nullptr;
        }
        return this->get<index>();
    }
    template<typename T>
    constexpr Option<const T&> get_if() const {
        constexpr size_t index = TypeList::Index<T, Types...>::value;
        static_assert(index != -1);
        if (m_value_index != index) {
            return nullptr;
        }
        return this->get<index>();
    }

    template<typename T>
    constexpr T get_or(T value) {
        constexpr size_t index = TypeList::Index<T, Types...>::value;
        static_assert(index != -1);
        if (m_value_index != index) {
            return value;
        }
        return this->get<index>();
    }

    template<size_t index>
    constexpr decltype(auto) get(in_place_index_t<index>) {
        return this->get<index>();
    }
    template<size_t index>
    constexpr decltype(auto) get(in_place_index_t<index>) const {
        return this->get<index>();
    }

    template<size_t index>
    constexpr decltype(auto) get() {
        assert(m_value_index == index);
        return m_storage.template access<index>();
    }

    template<size_t index>
    constexpr decltype(auto) get() const {
        assert(m_value_index == index);
        return m_storage.template access<index>();
    }

    template<typename T>
    constexpr T& as() {
        constexpr size_t index = TypeList::Index<T, Types...>::value;
        static_assert(index != -1);
        return this->get<index>();
    }
    template<typename T>
    constexpr const T& as() const {
        return const_cast<Variant&>(*this).as<T>();
    }

    template<size_t index>
    constexpr bool is() const {
        return m_value_index == index;
    }

    template<typename T>
    constexpr bool is() const {
        constexpr size_t index = TypeList::Index<T, Types...>::value;
        static_assert(index != -1);
        return this->is<index>();
    }

    template<typename Visitor>
    constexpr decltype(auto) visit(Visitor&& visitor) {
        return visit_by_index([&]<size_t index>(in_place_index_t<index>, auto&& self) {
            return forward<Visitor>(visitor)(self.template get<index>());
        });
    }

    template<typename Visitor>
    constexpr decltype(auto) visit(Visitor&& visitor) const {
        return visit_by_index([&]<size_t index>(in_place_index_t<index>, auto&& self) {
            return forward<Visitor>(visitor)(self.template get<index>());
        });
    }

    template<typename Visitor>
    constexpr decltype(auto) visit_by_index(Visitor&& visitor) {
        using R = typename InvokeResult<Visitor, in_place_index_t<0>, Variant>::type;
        auto table = details::CallTable<R, Visitor, Variant&>::build();
        return table.storage[m_value_index](forward<Visitor>(visitor), *this);
    }

    template<typename Visitor>
    constexpr decltype(auto) visit_by_index(Visitor&& visitor) const {
        using R = typename InvokeResult<Visitor, in_place_index_t<0>, Variant>::type;
        auto table = details::CallTable<R, Visitor, const Variant&>::build();
        return table.storage[m_value_index](forward<Visitor>(visitor), *this);
    }

    template<typename T, typename... Args, typename = typename EnableIf<TypeList::Index<T, Types...>::value != -1>::type>
    constexpr void emplace(Args&&... args) {
        this->destroy();
        constexpr size_t index = TypeList::Index<T, Types...>::value;
        m_storage.template emplace<index>(forward<Args>(args)...);
        m_value_index = index;
    }

    template<size_t index, typename... Args>
    constexpr void emplace(in_place_index_t<index>, Args&&... args) {
        this->destroy();
        m_storage.template emplace<index>(forward<Args>(args)...);
        m_value_index = index;
    }

    constexpr size_t index() const { return m_value_index; }

    constexpr void swap(Variant& other) {
        visit_by_index([&other]<size_t this_index>(in_place_index_t<this_index>, auto&& self) {
            other.template visit_by_index([&]<size_t other_index>(in_place_index_t<other_index>, auto&& other) {
                using T = TypeList::TypeAtIndex<this_index, Types...>::type;
                using U = TypeList::TypeAtIndex<other_index, Types...>::type;
                T temp = forward<T&&>(self.template get<this_index>());
                self.destroy();
                self.m_storage.template emplace<other_index>(forward<U&&>(other.template get<other_index>()));
                other.template destroy();
                other.m_storage.template emplace<this_index>(forward<T&&>(temp));
            });
        });

        LIIM::swap(this->m_value_index, other.m_value_index);
    }

    static constexpr size_t num_variants() { return TypeList::Count<Types...>::value; }

    constexpr bool operator!=(const Variant& other) const { return !(*this == other); }
    constexpr bool operator==(const Variant& other) const {
        return this->m_value_index == other.m_value_index && this->visit_by_index([&]<size_t index>(in_place_index_t<index>, auto&& self) {
            return self.template get<index>() == other.template get<index>();
        });
    }
    constexpr bool operator<=(const Variant& other) const {
        return LIIM::visit(
            [&](auto&& a, auto&& b) -> bool {
                return a <= b;
            },
            forward<const Variant>(*this), forward<const Variant>(other));
    }
    constexpr bool operator>=(const Variant& other) const {
        return LIIM::visit(
            [&](auto&& a, auto&& b) -> bool {
                return a >= b;
            },
            forward<const Variant>(*this), forward<const Variant>(other));
    }
    constexpr bool operator<(const Variant& other) const {
        return LIIM::visit(
            [&](auto&& a, auto&& b) -> bool {
                return a < b;
            },
            forward<const Variant>(*this), forward<const Variant>(other));
    }
    constexpr bool operator>(const Variant& other) const {
        return LIIM::visit(
            [&](auto&& a, auto&& b) -> bool {
                return a > b;
            },
            forward<const Variant>(*this), forward<const Variant>(other));
    }

private:
    template<typename Visitor, typename... Variants>
    friend inline constexpr decltype(auto) visit(Visitor&&, Variants&&...);

    constexpr void destroy() {
        this->visit_by_index([]<size_t index>(in_place_index_t<index>, auto&& self) {
            self.m_storage.template destroy<index>();
        });
    }

    details::Union<Types...> m_storage;
    size_t m_value_index { 0 };
};

template<typename... Types>
constexpr void swap(Variant<Types...>& a, Variant<Types...>& b) {
    a.swap(b);
}

struct Monostate {
    constexpr bool operator==(const Monostate&) const { return true; }
    constexpr bool operator!=(const Monostate&) const { return false; }
};
}

using LIIM::Monostate;
using LIIM::Variant;
