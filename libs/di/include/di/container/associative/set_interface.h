#pragma once

#include "di/container/action/sequence.h"
#include "di/container/concepts/prelude.h"
#include "di/container/interface/erase.h"
#include "di/container/interface/prelude.h"
#include "di/container/iterator/prelude.h"
#include "di/container/meta/prelude.h"
#include "di/container/types/prelude.h"
#include "di/math/to_unsigned.h"
#include "di/meta/core.h"
#include "di/util/clone.h"
#include "di/vocab/optional/prelude.h"

namespace di::container {
template<typename Self, typename Value, typename Iterator, typename ConstIterator,
         template<typename> typename ValidForLookup, bool is_multi>
class SetInterface {
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

    constexpr auto size() const -> size_t { return self().size(); }

    template<concepts::ContainerCompatible<Value> Con, typename... Args>
    requires(concepts::ConstructibleFrom<Self, Args...>)
    constexpr friend auto tag_invoke(types::Tag<util::create_in_place>, InPlaceType<Self>, Con&& container,
                                     Args&&... args) {
        auto result = Self(util::forward<Args>(args)...);
        result.insert_container(util::forward<Con>(container));
        return result;
    }

public:
    constexpr auto empty() const -> bool { return size() == 0; }

    constexpr void clear() { erase(this->begin(), this->end()); }

    constexpr auto insert(Value const& value)
    requires(concepts::Clonable<Value>)
    {
        return self().insert_with_factory(value, [&] {
            return util::clone(value);
        });
    }

    constexpr auto insert(Value&& value) {
        return self().insert_with_factory(value, [&] {
            return util::move(value);
        });
    }

    template<typename U>
    requires(valid<U> && concepts::CreatableFrom<Value, U>)
    constexpr auto insert(U&& value) {
        return self().insert_with_factory(value, [&] {
            return util::create<Value>(util::forward<U>(value));
        });
    }

    constexpr auto insert(ConstIterator hint, Value const& value)
    requires(concepts::Clonable<Value>)
    {
        return self().insert_with_factory(hint, value, [&] {
            return util::clone(value);
        });
    }

    constexpr auto insert(ConstIterator hint, Value&& value) {
        return self().insert_with_factory(hint, value, [&] {
            return util::move(value);
        });
    }

    template<typename U>
    requires(valid<U> && concepts::CreatableFrom<Value, U>)
    constexpr auto insert(ConstIterator hint, U&& value) {
        return self().insert_with_factory(hint, value, [&] {
            return util::create<Value>(util::forward<U>(value));
        });
    }

    template<typename... Args>
    requires(concepts::ConstructibleFrom<Value, Args...>)
    constexpr auto emplace(Args&&... args) {
        return insert(Value(util::forward<Args>(args)...));
    }

    template<typename... Args>
    requires(concepts::ConstructibleFrom<Value, Args...>)
    constexpr auto emplace_hint(ConstIterator hint, Args&&... args) {
        return insert(util::move(hint), Value(util::forward<Args>(args)...));
    }

    template<concepts::ContainerCompatible<Value> Con>
    constexpr auto insert_container(Con&& container) {
        if constexpr (concepts::Expected<decltype(insert(*container::begin(container)))>) {
            auto temp_container = Self {};
            return container::sequence(util::forward<Con>(container),
                                       [&]<typename X>(X&& value) {
                                           return temp_container.insert(util::forward<X>(value));
                                       }) >>
                   [&] {
                       return invoke_as_fallible([&] {
                           return self().merge_impl(util::move(temp_container));
                       });
                   };
        } else {
            auto first = container::begin(container);
            auto last = container::end(container);
            for (; first != last; ++first) {
                insert(*first);
            }
        }
    }

    template<concepts::ContainerCompatible<Value> Con>
    constexpr void insert_container(ConstIterator hint, Con&& container) {
        if constexpr (concepts::Expected<decltype(insert(hint, *container::begin(container)))>) {
            return insert_container(util::forward<Con>(container));
        } else {
            auto first = container::begin(container);
            auto last = container::end(container);
            for (; first != last; ++first) {
                hint = insert(hint, *first);
            }
        }
    }

    constexpr auto merge(Self& self) { return self().merge_impl(util::move(self)); }
    constexpr auto merge(Self&& self) { return self().merge_impl(util::move(self)); }

    constexpr auto erase(Iterator position) { return self().erase_impl(util::move(position)); }

    constexpr auto erase(Iterator first, Iterator last) -> Iterator {
        while (first != last) {
            first = self().erase_impl(first);
        }
        return last;
    }

    constexpr auto erase(Value const& needle) -> size_t {
        if constexpr (!is_multi) {
            auto it = this->find(needle);
            if (it == end()) {
                return 0;
            }
            self().erase_impl(util::move(it));
            return 1;
        } else {
            auto [first, last] = this->equal_range(needle);
            size_t result = 0;
            for (; first != last; ++result) {
                first = self().erase_impl(first);
            }
            return result;
        }
    }

    template<typename U>
    requires(valid<U>)
    constexpr auto erase(U&& needle) -> size_t {
        if constexpr (!is_multi) {
            auto it = this->find(needle);
            if (it == end()) {
                return 0;
            }
            self().erase_impl(util::move(it));
            return 1;
        } else {
            auto [first, last] = this->equal_range(needle);
            size_t result = 0;
            for (; first != last; ++result) {
                first = self().erase_impl(first);
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

    constexpr auto count(Value const& needle) const -> size_t {
        if constexpr (!is_multi) {
            return this->contains(needle) ? 1 : 0;
        } else {
            return math::to_unsigned(container::distance(this->equal_range(needle)));
        }
    }

    template<typename U>
    requires(valid<U>)
    constexpr auto count(U&& needle) const -> size_t {
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
    // Set union.
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

    // Set intersection.
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

    // Set differerce.
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
        auto result = 0ZU;
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
