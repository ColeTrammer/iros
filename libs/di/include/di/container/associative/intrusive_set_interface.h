#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/interface/erase.h>
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

    constexpr auto self() -> Self& { return static_cast<Self&>(*this); }
    constexpr auto self() const -> Self const& { return static_cast<Self const&>(*this); }

    constexpr auto unconst_iterator(ConstIterator it) -> Iterator { return self().unconst_iterator(util::move(it)); }

    constexpr auto begin() -> Iterator { return self().begin(); }
    constexpr auto end() -> Iterator { return self().end(); }

    constexpr auto begin() const -> ConstIterator { return self().begin(); }
    constexpr auto end() const -> ConstIterator { return self().end(); }

    constexpr auto size() const -> usize
    requires(requires { self().size(); })
    {
        return self().size();
    }

public:
    constexpr auto empty() const -> bool { return self().empty(); }

    constexpr void clear() { erase(begin(), end()); }

    constexpr auto insert(Node& node) { return self().insert_node(node); }

    constexpr auto insert(ConstIterator hint, Node& node) { return self().insert_node(hint, node); }

    constexpr auto merge(Self& self) { return self().merge_impl(util::move(self)); }
    constexpr auto merge(Self&& self) { return self().merge_impl(util::move(self)); }

    constexpr auto erase(Iterator position) { return self().erase_impl(util::move(position)); }

    constexpr auto erase(Iterator first, Iterator last) -> Iterator {
        while (first != last) {
            self().erase_impl(first++);
        }
        return last;
    }

    constexpr auto erase(Value const& needle) -> usize {
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
    constexpr auto erase(U&& needle) -> usize {
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

    constexpr auto front() -> Optional<Value&> {
        return lift_bool(!empty()) % [&] {
            return util::ref(*begin());
        };
    }
    constexpr auto front() const -> Optional<Value const&> {
        return lift_bool(!empty()) % [&] {
            return util::cref(*begin());
        };
    }

    constexpr auto back() -> Optional<Value&> {
        return lift_bool(!empty()) % [&] {
            return util::ref(*container::prev(end()));
        };
    }
    constexpr auto back() const -> Optional<Value const&> {
        return lift_bool(!empty()) % [&] {
            return util::cref(*container::prev(end()));
        };
    }

    constexpr auto at(Value const& needle) -> Optional<Value&> {
        auto it = this->find(needle);
        return lift_bool(it != end()) % [&] {
            return util::ref(*it);
        };
    }
    constexpr auto at(Value const& needle) const -> Optional<Value const&> {
        auto it = this->find(needle);
        return lift_bool(it != end()) % [&] {
            return util::cref(*it);
        };
    }

    template<typename U>
    requires(valid<U>)
    constexpr auto at(U&& needle) -> Optional<Value&> {
        auto it = this->find(needle);
        return lift_bool(it != end()) % [&] {
            return util::ref(*it);
        };
    }
    template<typename U>
    requires(valid<U>)
    constexpr auto at(U&& needle) const -> Optional<Value const&> {
        auto it = this->find(needle);
        return lift_bool(it != end()) % [&] {
            return util::cref(*it);
        };
    }

    constexpr auto find(Value const& needle) -> Iterator { return unconst_iterator(self().find_impl(needle)); }
    constexpr auto find(Value const& needle) const -> ConstIterator { return self().find_impl(needle); }

    template<typename U>
    requires(valid<U>)
    constexpr auto find(U&& needle) -> Iterator {
        return unconst_iterator(self().find_impl(needle));
    }
    template<typename U>
    requires(valid<U>)
    constexpr auto find(U&& needle) const -> ConstIterator {
        return self().find_impl(needle);
    }

    constexpr auto contains(Value const& needle) const -> bool { return this->find(needle) != end(); }
    template<typename U>
    requires(valid<U>)
    constexpr auto contains(U&& needle) const -> bool {
        return this->find(needle) != end();
    }

    constexpr auto count(Value const& needle) const -> usize {
        return math::to_unsigned(container::distance(this->equal_range(needle)));
    }

    template<typename U>
    requires(valid<U>)
    constexpr auto count(U&& needle) const -> usize {
        if constexpr (!is_multi) {
            return this->contains(needle) ? 1 : 0;
        } else {
            return math::to_unsigned(container::distance(this->equal_range(needle)));
        }
    }

    constexpr auto equal_range(Value const& needle) -> View<Iterator> {
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
    constexpr auto equal_range(U&& needle) -> View<Iterator> {
        if constexpr (!is_multi) {
            auto it = this->find(needle);
            return { it, container::next(it, 1, end()) };
        } else {
            return self().equal_range_impl(needle);
        }
    }

    constexpr auto equal_range(Value const& needle) const -> View<ConstIterator> {
        if constexpr (!is_multi) {
            auto it = this->find(needle);
            return { it, container::next(it, 1, end()) };
        } else {
            return self().equal_range_impl(needle);
        }
    }
    template<typename U>
    requires(valid<U>)
    constexpr auto equal_range(U&& needle) const -> View<ConstIterator> {
        if constexpr (!is_multi) {
            auto it = this->find(needle);
            return { it, container::next(it, 1, end()) };
        } else {
            return self().equal_range_impl(needle);
        }
    }

    constexpr auto lower_bound(Value const& needle) -> Iterator
    requires(requires {
        { self().lower_bound_impl(needle) } -> concepts::SameAs<ConstIterator>;
    })
    {
        return unconst_iterator(self().lower_bound_impl(needle));
    }
    constexpr auto lower_bound(Value const& needle) const -> ConstIterator
    requires(requires {
        { self().lower_bound_impl(needle) } -> concepts::SameAs<ConstIterator>;
    })
    {
        return self().lower_bound_impl(needle);
    }

    template<typename U>
    requires(valid<U>)
    constexpr auto lower_bound(U&& needle) -> Iterator
    requires(requires {
        { self().lower_bound_impl(needle) } -> concepts::SameAs<ConstIterator>;
    })
    {
        return unconst_iterator(self().lower_bound_impl(needle));
    }
    template<typename U>
    requires(valid<U>)
    constexpr auto lower_bound(U&& needle) const -> ConstIterator
    requires(requires {
        { self().lower_bound_impl(needle) } -> concepts::SameAs<ConstIterator>;
    })
    {
        return self().lower_bound_impl(needle);
    }

    constexpr auto upper_bound(Value const& needle) -> Iterator
    requires(requires {
        { self().upper_bound_impl(needle) } -> concepts::SameAs<ConstIterator>;
    })
    {
        return unconst_iterator(self().upper_bound_impl(needle));
    }
    constexpr auto upper_bound(Value const& needle) const -> ConstIterator
    requires(requires {
        { self().upper_bound_impl(needle) } -> concepts::SameAs<ConstIterator>;
    })
    {
        return self().upper_bound_impl(needle);
    }

    template<typename U>
    requires(valid<U>)
    constexpr auto upper_bound(U&& needle) -> Iterator
    requires(requires {
        { self().upper_bound_impl(needle) } -> concepts::SameAs<ConstIterator>;
    })
    {
        return unconst_iterator(self().upper_bound_impl(needle));
    }
    template<typename U>
    requires(valid<U>)
    constexpr auto upper_bound(U&& needle) const -> ConstIterator
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

    constexpr friend auto operator|=(Self& a, Self&& b) -> decltype(auto) {
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

    constexpr friend auto operator&=(Self& a, Self const& b) -> Self& requires(!is_multi) {
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

    constexpr friend auto operator-=(Self& a, Self const& b) -> Self& requires(!is_multi) {
        a.subtract(b);
        return a;
    }

    template<typename F, SameAs<Tag<erase_if>> T = Tag<erase_if>>
    requires(concepts::Predicate<F&, Value const&>)
    constexpr friend auto tag_invoke(T, Self& self, F&& function) -> usize {
        auto it = self.begin();
        auto result = 0zu;
        while (it != self.end()) {
            if (function(*it)) {
                it = self.erase(it);
                ++result;
            } else {
                ++it;
            }
        }
        return result;
    }
};
}
