#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/interface/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/container/types/prelude.h>
#include <di/math/to_unsigned.h>
#include <di/util/clone.h>
#include <di/vocab/optional/prelude.h>

namespace di::container {
template<typename Self, typename Value, typename Iterator, typename ConstIterator,
         template<typename> typename ValidForLookup, bool is_multi>
class MapInterface {
private:
    using Key = meta::TupleElement<Value, 0>;
    using Val = meta::TupleElement<Value, 1>;

    template<typename T>
    constexpr static bool valid = ValidForLookup<T>::value;

    constexpr Self& self() { return static_cast<Self&>(*this); }
    constexpr Self const& self() const { return static_cast<Self const&>(*this); }

    constexpr Iterator unconst_iterator(ConstIterator it) { return self().unconst_iterator(util::move(it)); }

    constexpr Iterator begin() { return self().begin(); }
    constexpr Iterator end() { return self().end(); }

    constexpr ConstIterator begin() const { return self().begin(); }
    constexpr ConstIterator end() const { return self().end(); }

    constexpr size_t size() const { return self().size(); }

    template<concepts::ContainerCompatible<Value> Con, typename... Args>
    requires(concepts::ConstructibleFrom<Self, Args...>)
    constexpr friend auto tag_invoke(types::Tag<util::create_in_place>, InPlaceType<Self>, Con&& container,
                                     Args&&... args) {
        auto result = Self(util::forward<Args>(args)...);
        result.insert_container(util::forward<Con>(container));
        return result;
    }

public:
    constexpr bool empty() const { return size() == 0; }

    constexpr void clear() { erase(begin(), end()); }

    constexpr decltype(auto) operator[](Key const& needle)
    requires(!is_multi && concepts::Clonable<Key> && concepts::DefaultConstructible<Val>)
    {
        return as_fallible(this->try_emplace(needle)) % [&](auto result) {
            return util::ref(util::get<1>(*util::get<0>(result)));
        } | try_infallible;
    }

    constexpr decltype(auto) operator[](Key&& needle)
    requires(!is_multi && concepts::DefaultConstructible<Val>)
    {
        return as_fallible(this->try_emplace(util::move(needle))) % [&](auto result) {
            return util::ref(util::get<1>(*util::get<0>(result)));
        } | try_infallible;
    }

    template<typename K>
    requires(!is_multi && concepts::CreatableFrom<Key, K> && concepts::DefaultConstructible<Val>)
    constexpr decltype(auto) operator[](K&& needle) {
        return as_fallible(this->try_emplace(util::forward<K>(needle))) % [&](auto result) {
            return util::ref(util::get<1>(*util::get<0>(result)));
        } | try_infallible;
    }

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

    template<typename U>
    requires(!is_multi && concepts::Clonable<Key> && concepts::CreatableFrom<Val, U>)
    constexpr auto insert_or_assign(Key const& needle, U&& value) {
        bool did_insert = false;
        return as_fallible(self().insert_with_factory(
                   needle,
                   [&] {
                       did_insert = true;
                       return as_fallible(util::clone(needle)) % [&](Key&& key) {
                           return as_fallible(util::create<Val>(util::forward<U>(value))) % [&](Val&& value) {
                               return Value(util::move(key), util::move(value));
                           };
                       } | try_infallible;
                   })) |
               if_success([&](auto&& result) {
                   if (!did_insert) {
                       util::get<1>(*util::get<0>(result)) = Val(util::forward<U>(value));
                   }
               }) |
               try_infallible;
    }

    template<typename U>
    requires(!is_multi && concepts::CreatableFrom<Val, U>)
    constexpr auto insert_or_assign(Key&& needle, U&& value) {
        bool did_insert = false;
        return as_fallible(self().insert_with_factory(
                   needle,
                   [&] {
                       did_insert = true;
                       return as_fallible(util::create<Val>(util::forward<U>(value))) % [&](Val&& value) {
                           return Value(util::move(needle), util::move(value));
                       } | try_infallible;
                   })) |
               if_success([&](auto&& result) {
                   if (!did_insert) {
                       util::get<1>(*util::get<0>(result)) = Val(util::forward<U>(value));
                   }
               }) |
               try_infallible;
    }

    template<typename K, typename U>
    requires(!is_multi && valid<K> && concepts::CreatableFrom<Key, K> && concepts::CreatableFrom<Val, U>)
    constexpr auto insert_or_assign(K&& needle, U&& value) {
        bool did_insert = false;
        return as_fallible(self().insert_with_factory(
                   needle,
                   [&] {
                       did_insert = true;
                       return as_fallible(util::create<Key>(needle)) % [&](Key&& key) {
                           return as_fallible(util::create<Val>(util::forward<U>(value))) % [&](Val&& value) {
                               return Value(util::move(key), util::move(value));
                           };
                       } | try_infallible;
                   })) |
               if_success([&](auto&& result) {
                   if (!did_insert) {
                       util::get<1>(*util::get<0>(result)) = Val(util::forward<U>(value));
                   }
               }) |
               try_infallible;
    }

    template<typename U>
    requires(!is_multi && concepts::Clonable<Key> && concepts::CreatableFrom<Val, U>)
    constexpr auto insert_or_assign(ConstIterator hint, Key const& needle, U&& value) {
        bool did_insert = false;
        return as_fallible(self().insert_with_factory(
                   hint, needle,
                   [&] {
                       did_insert = true;
                       return as_fallible(util::clone(needle)) % [&](Key&& key) {
                           return as_fallible(util::create<Val>(util::forward<U>(value))) % [&](Val&& value) {
                               return Value(util::move(key), util::move(value));
                           };
                       } | try_infallible;
                   })) |
               if_success([&](auto&& result) {
                   if (!did_insert) {
                       util::get<1>(*result) = Val(util::forward<U>(value));
                   }
               }) |
               try_infallible;
    }

    template<typename U>
    requires(!is_multi && concepts::CreatableFrom<Val, U>)
    constexpr auto insert_or_assign(ConstIterator hint, Key&& needle, U&& value) {
        bool did_insert = false;
        return as_fallible(self().insert_with_factory(
                   hint, needle,
                   [&] {
                       did_insert = true;
                       return as_fallible(util::create<Val>(util::forward<U>(value))) % [&](Val&& value) {
                           return Value(util::move(needle), util::move(value));
                       } | try_infallible;
                   })) |
               if_success([&](auto&& result) {
                   if (!did_insert) {
                       util::get<1>(*result) = Val(util::forward<U>(value));
                   }
               }) |
               try_infallible;
    }

    template<typename K, typename U>
    requires(!is_multi && valid<K> && concepts::CreatableFrom<Key, K> && concepts::CreatableFrom<Val, U>)
    constexpr auto insert_or_assign(ConstIterator hint, K&& needle, U&& value) {
        bool did_insert = false;
        return as_fallible(self().insert_with_factory(
                   needle,
                   [&] {
                       did_insert = true;
                       return as_fallible(util::create<Key>(needle)) >> [&](Key&& key) {
                           return as_fallible(util::create<Val>(util::forward<U>(value))) % [&](Val&& value) {
                               return Value(util::move(key), util::move(value));
                           };
                       } | try_infallible;
                   })) |
               if_success([&](auto&& result) {
                   if (!did_insert) {
                       util::get<1>(*result) = Val(util::forward<U>(value));
                   }
               }) |
               try_infallible;
    }

    template<typename... Args>
    requires(!is_multi && concepts::Clonable<Key> && concepts::CreatableFrom<Val, Args...>)
    constexpr auto try_emplace(Key const& needle, Args&&... args) {
        return self().insert_with_factory(needle, [&] {
            return as_fallible(util::clone(needle)) >> [&](Key&& key) {
                return as_fallible(util::create<Val>(util::forward<Args>(args)...)) % [&](Val&& value) {
                    return Value(util::move(key), util::move(value));
                };
            } | try_infallible;
        });
    }

    template<typename... Args>
    requires(!is_multi && concepts::CreatableFrom<Val, Args...>)
    constexpr auto try_emplace(Key&& needle, Args&&... args) {
        return self().insert_with_factory(needle, [&] {
            return as_fallible(util::create<Val>(util::forward<Args>(args)...)) % [&](Val&& value) {
                return Value(util::move(needle), util::move(value));
            } | try_infallible;
        });
    }

    template<typename K, typename... Args>
    requires(!is_multi && concepts::CreatableFrom<Key, K> && concepts::CreatableFrom<Val, Args...>)
    constexpr auto try_emplace(K&& needle, Args&&... args) {
        return self().insert_with_factory(needle, [&] {
            return as_fallible(util::create<Key>(util::forward<K>(needle))) >> [&](Key&& key) {
                return as_fallible(util::create<Val>(util::forward<Args>(args)...)) % [&](Val&& value) {
                    return Value(util::move(key), util::move(value));
                };
            } | try_infallible;
        });
    }

    template<typename... Args>
    requires(!is_multi && concepts::Clonable<Key> && concepts::CreatableFrom<Val, Args...>)
    constexpr auto try_emplace(ConstIterator hint, Key const& needle, Args&&... args) {
        return self().insert_with_factory(hint, needle, [&] {
            return as_fallible(util::clone(needle)) >> [&](Key&& key) {
                return as_fallible(util::create<Val>(util::forward<Args>(args)...)) % [&](Val&& value) {
                    return Value(util::move(key), util::move(value));
                };
            } | try_infallible;
        });
    }

    template<typename... Args>
    requires(!is_multi && concepts::CreatableFrom<Val, Args...>)
    constexpr auto try_emplace(ConstIterator hint, Key&& needle, Args&&... args) {
        return self().insert_with_factory(hint, needle, [&] {
            return as_fallible(util::create<Val>(util::forward<Args>(args)...)) % [&](Val&& value) {
                return Value(util::move(needle), util::move(value));
            } | try_infallible;
        });
    }

    template<typename K, typename... Args>
    requires(!is_multi && concepts::CreatableFrom<Key, K> && concepts::CreatableFrom<Val, Args...>)
    constexpr auto try_emplace(ConstIterator hint, K&& needle, Args&&... args) {
        return self().insert_with_factory(hint, needle, [&] {
            return as_fallible(util::create<Key>(util::forward<K>(needle))) >> [&](Key&& key) {
                return as_fallible(util::create<Val>(util::forward<Args>(args)...)) % [&](Val&& value) {
                    return Value(util::move(key), util::move(value));
                };
            } | try_infallible;
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
        return insert(hint, Value(util::forward<Args>(args)...));
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

    constexpr Iterator erase(Iterator first, Iterator last) {
        while (first != last) {
            self().erase_impl(first++);
        }
        return last;
    }

    constexpr size_t erase(Key const& needle) {
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
                self().erase_impl(++first);
            }
            return result;
        }
    }

    template<typename U>
    requires(valid<U>)
    constexpr size_t erase(U&& needle) {
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

    constexpr Optional<Val&> at(Key const& needle) {
        auto it = this->find(needle);
        return lift_bool(it != end()) % [&] {
            return util::ref(util::get<1>(*it));
        };
    }
    constexpr Optional<Val const&> at(Key const& needle) const {
        auto it = this->find(needle);
        return lift_bool(it != end()) % [&] {
            return util::cref(util::get<1>(*it));
        };
    }

    template<typename U>
    requires(valid<U>)
    constexpr Optional<Val&> at(U&& needle) {
        auto it = this->find(needle);
        return lift_bool(it != end()) % [&] {
            return util::ref(util::get<1>(*it));
        };
    }
    template<typename U>
    requires(valid<U>)
    constexpr Optional<Val const&> at(U&& needle) const {
        auto it = this->find(needle);
        return lift_bool(it != end()) % [&] {
            return util::cref(util::get<1>(*it));
        };
    }

    constexpr Iterator find(Key const& needle) { return unconst_iterator(self().find_impl(needle)); }
    constexpr ConstIterator find(Key const& needle) const { return self().find_impl(needle); }

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

    constexpr bool contains(Key const& needle) const { return this->find(needle) != end(); }
    template<typename U>
    requires(valid<U>)
    constexpr bool contains(U&& needle) const {
        return this->find(needle) != end();
    }

    constexpr size_t count(Key const& needle) const {
        if constexpr (!is_multi) {
            return this->contains(needle) ? 1 : 0;
        } else {
            return math::to_unsigned(container::distance(this->equal_range(needle)));
        }
    }

    template<typename U>
    requires(valid<U>)
    constexpr size_t count(U&& needle) const {
        if constexpr (!is_multi) {
            return this->contains(needle) ? 1 : 0;
        } else {
            return math::to_unsigned(container::distance(this->equal_range(needle)));
        }
    }

    constexpr View<Iterator> equal_range(Key const& needle) {
        if constexpr (!is_multi) {
            auto it = this->find(needle);
            return { it, container::next(it, 1, end()) };
        } else {
            auto [start, last] = self().equal_range_impl(needle);
            return { unconst_iterator(util::move(start)), unconst_iterator(util::move(end)) };
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

    constexpr View<ConstIterator> equal_range(Key const& needle) const {
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

    constexpr Iterator lower_bound(Key const& needle)
    requires(requires {
                 { self().lower_bound_impl(needle) } -> concepts::SameAs<ConstIterator>;
             })
    {
        return unconst_iterator(self().lower_bound_impl(needle));
    }
    constexpr ConstIterator lower_bound(Key const& needle) const
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

    constexpr Iterator upper_bound(Key const& needle)
    requires(requires {
                 { self().upper_bound_impl(needle) } -> concepts::SameAs<ConstIterator>;
             })
    {
        return unconst_iterator(self().upper_bound_impl(needle));
    }
    constexpr ConstIterator upper_bound(Key const& needle) const
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
};
}
