#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/interface/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/container/types/prelude.h>
#include <di/math/to_unsigned.h>
#include <di/vocab/expected/prelude.h>
#include <di/vocab/optional/prelude.h>

namespace di::container {
template<typename Self, typename Value, typename Node, typename Iterator, typename ConstIterator,
         template<typename> typename ValidForLookup, bool is_multi>
class IntrusiveSetInterface {
private:
    template<typename T>
    constexpr static bool valid = ValidForLookup<T>::value;

    constexpr Self& self() { return static_cast<Self&>(*this); }
    constexpr Self const& self() const { return static_cast<Self const&>(*this); }

    constexpr Iterator unconst_iterator(ConstIterator it) { return self().unconst_iterator(util::move(it)); }

    constexpr Iterator begin() { return self().begin(); }
    constexpr Iterator end() { return self().end(); }

    constexpr ConstIterator begin() const { return self().begin(); }
    constexpr ConstIterator end() const { return self().end(); }

    constexpr usize size() const
    requires(requires { self().size(); })
    {
        return self().size();
    }

public:
    constexpr bool empty() const { return self().empty(); }

    constexpr void clear() { erase(begin(), end()); }

    constexpr auto insert(Node& node) { return self().insert_node(node); }

    constexpr auto insert(ConstIterator hint, Node& node) { return self().insert_node(hint, node); }

    constexpr auto merge(Self& self) { return self().merge_impl(util::move(self)); }
    constexpr auto merge(Self&& self) { return self().merge_impl(util::move(self)); }

    constexpr auto erase(Iterator position) { return self().erase_impl(util::move(position)); }

    constexpr Iterator erase(Iterator first, Iterator last) {
        while (first != last) {
            self().erase_impl(first++);
        }
        return last;
    }

    constexpr usize erase(Value const& needle) {
        if constexpr (!is_multi) {
            auto it = this->find(needle);
            if (it == end()) {
                return 0;
            }
            self().erase_impl(util::move(it));
            return 1;
        } else {
            auto [first, last] = this->equal_range(needle);
            usize result = 0;
            for (; first != last; ++result) {
                self().erase_impl(++first);
            }
            return result;
        }
    }

    template<typename U>
    requires(valid<U>)
    constexpr usize erase(U&& needle) {
        if constexpr (!is_multi) {
            auto it = this->find(needle);
            if (it == end()) {
                return 0;
            }
            self().erase_impl(util::move(it));
            return 1;
        } else {
            auto [first, last] = this->equal_range(needle);
            usize result = 0;
            for (; first != last; ++result) {
                self().erase_impl(++first);
            }
            return result;
        }
    }

    constexpr Optional<Value&> front() {
        return lift_bool(!empty()) % [&] {
            return util::ref(*begin());
        };
    }
    constexpr Optional<Value const&> front() const {
        return lift_bool(!empty()) % [&] {
            return util::cref(*begin());
        };
    }

    constexpr Optional<Value&> back() {
        return lift_bool(!empty()) % [&] {
            return util::ref(*container::prev(end()));
        };
    }
    constexpr Optional<Value const&> back() const {
        return lift_bool(!empty()) % [&] {
            return util::cref(*container::prev(end()));
        };
    }

    constexpr Optional<Value&> at(Value const& needle) {
        auto it = this->find(needle);
        return lift_bool(it != end()) % [&] {
            return util::ref(*it);
        };
    }
    constexpr Optional<Value const&> at(Value const& needle) const {
        auto it = this->find(needle);
        return lift_bool(it != end()) % [&] {
            return util::cref(*it);
        };
    }

    template<typename U>
    requires(valid<U>)
    constexpr Optional<Value&> at(U&& needle) {
        auto it = this->find(needle);
        return lift_bool(it != end()) % [&] {
            return util::ref(*it);
        };
    }
    template<typename U>
    requires(valid<U>)
    constexpr Optional<Value const&> at(U&& needle) const {
        auto it = this->find(needle);
        return lift_bool(it != end()) % [&] {
            return util::cref(*it);
        };
    }

    constexpr Iterator find(Value const& needle) { return unconst_iterator(self().find_impl(needle)); }
    constexpr ConstIterator find(Value const& needle) const { return self().find_impl(needle); }

    template<typename U>
    requires(valid<U>)
    constexpr Iterator find(U&& needle) {
        return unconst_iterator(self().find_impl(needle));
    }
    template<typename U>
    requires(valid<U>)
    constexpr ConstIterator find(U&& needle) const {
        return self().find_impl(needle);
    }

    constexpr bool contains(Value const& needle) const { return this->find(needle) != end(); }
    template<typename U>
    requires(valid<U>)
    constexpr bool contains(U&& needle) const {
        return this->find(needle) != end();
    }

    constexpr usize count(Value const& needle) const {
        return math::to_unsigned(container::distance(this->equal_range(needle)));
    }

    template<typename U>
    requires(valid<U>)
    constexpr usize count(U&& needle) const {
        if constexpr (!is_multi) {
            return this->contains(needle) ? 1 : 0;
        } else {
            return math::to_unsigned(container::distance(this->equal_range(needle)));
        }
    }

    constexpr View<Iterator> equal_range(Value const& needle) {
        if constexpr (!is_multi) {
            auto it = this->find(needle);
            return { it, container::next(it, 1, end()) };
        } else {
            auto [start, last] = self().equal_range_impl(needle);
            return { unconst_iterator(util::move(start)), unconst_iterator(util::move(last)) };
        }
    }
    template<typename U>
    requires(valid<U>)
    constexpr View<Iterator> equal_range(U&& needle) {
        if constexpr (!is_multi) {
            auto it = this->find(needle);
            return { it, container::next(it, 1, end()) };
        } else {
            return self().equal_range_impl(needle);
        }
    }

    constexpr View<ConstIterator> equal_range(Value const& needle) const {
        if constexpr (!is_multi) {
            auto it = this->find(needle);
            return { it, container::next(it, 1, end()) };
        } else {
            return self().equal_range_impl(needle);
        }
    }
    template<typename U>
    requires(valid<U>)
    constexpr View<ConstIterator> equal_range(U&& needle) const {
        if constexpr (!is_multi) {
            auto it = this->find(needle);
            return { it, container::next(it, 1, end()) };
        } else {
            return self().equal_range_impl(needle);
        }
    }

    constexpr Iterator lower_bound(Value const& needle)
    requires(requires {
        { self().lower_bound_impl(needle) } -> concepts::SameAs<ConstIterator>;
    })
    {
        return unconst_iterator(self().lower_bound_impl(needle));
    }
    constexpr ConstIterator lower_bound(Value const& needle) const
    requires(requires {
        { self().lower_bound_impl(needle) } -> concepts::SameAs<ConstIterator>;
    })
    {
        return self().lower_bound_impl(needle);
    }

    template<typename U>
    requires(valid<U>)
    constexpr Iterator lower_bound(U&& needle)
    requires(requires {
        { self().lower_bound_impl(needle) } -> concepts::SameAs<ConstIterator>;
    })
    {
        return unconst_iterator(self().lower_bound_impl(needle));
    }
    template<typename U>
    requires(valid<U>)
    constexpr ConstIterator lower_bound(U&& needle) const
    requires(requires {
        { self().lower_bound_impl(needle) } -> concepts::SameAs<ConstIterator>;
    })
    {
        return self().lower_bound_impl(needle);
    }

    constexpr Iterator upper_bound(Value const& needle)
    requires(requires {
        { self().upper_bound_impl(needle) } -> concepts::SameAs<ConstIterator>;
    })
    {
        return unconst_iterator(self().upper_bound_impl(needle));
    }
    constexpr ConstIterator upper_bound(Value const& needle) const
    requires(requires {
        { self().upper_bound_impl(needle) } -> concepts::SameAs<ConstIterator>;
    })
    {
        return self().upper_bound_impl(needle);
    }

    template<typename U>
    requires(valid<U>)
    constexpr Iterator upper_bound(U&& needle)
    requires(requires {
        { self().upper_bound_impl(needle) } -> concepts::SameAs<ConstIterator>;
    })
    {
        return unconst_iterator(self().upper_bound_impl(needle));
    }
    template<typename U>
    requires(valid<U>)
    constexpr ConstIterator upper_bound(U&& needle) const
    requires(requires {
        { self().upper_bound_impl(needle) } -> concepts::SameAs<ConstIterator>;
    })
    {
        return self().upper_bound_impl(needle);
    }

    constexpr void intersect(Self const& b)
    requires(!is_multi)
    {
        auto it = begin();
        auto last = end();
        while (it != last) {
            auto save = it++;
            if (!b.contains(*save)) {
                erase(save);
            }
        }
    }

    constexpr void subtract(Self const& b)
    requires(!is_multi)
    {
        auto it = begin();
        auto last = end();
        while (it != last) {
            auto save = it++;
            if (b.contains(*save)) {
                erase(save);
            }
        }
    }

private:
    /// Set union.
    constexpr friend auto operator|(Self&& a, Self&& b) {
        return invoke_as_fallible([&] {
                   return a.merge(util::move(b));
               }) |
               [&] {
                   return util::move(a);
               } |
               try_infallible;
    }

    constexpr friend decltype(auto) operator|=(Self& a, Self&& b) {
        return invoke_as_fallible([&] {
                   return a.merge(util::move(b));
               }) |
               [&] {
                   return util::ref(a);
               } |
               try_infallible;
    }

    /// Set intersection.
    constexpr friend auto operator&(Self&& a, Self const& b)
    requires(!is_multi)
    {
        a.intersect(b);
        return util::move(a);
    }

    constexpr friend Self& operator&=(Self& a, Self const& b)
    requires(!is_multi)
    {
        a.intersect(b);
        return a;
    }

    /// Set differerce.
    constexpr friend auto operator-(Self&& a, Self const& b)
    requires(!is_multi)
    {
        a.subtract(b);
        return util::move(a);
    }

    constexpr friend Self& operator-=(Self& a, Self const& b)
    requires(!is_multi)
    {
        a.subtract(b);
        return a;
    }
};
}
