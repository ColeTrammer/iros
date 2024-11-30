#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/iterator/iterator_base.h>
#include <di/container/meta/prelude.h>
#include <di/vocab/variant/prelude.h>

namespace di::container {
template<concepts::InputIterator Iter, concepts::SentinelFor<Iter> Sent>
requires(!concepts::SameAs<Iter, Sent> && concepts::Copyable<Iter>)
class CommonIterator
    : public IteratorBase<CommonIterator<Iter, Sent>,
                          meta::Conditional<concepts::ForwardIterator<Iter>, ForwardIteratorTag, InputIteratorTag>,
                          meta::IteratorValue<Iter>, meta::IteratorSSizeType<Iter>> {
private:
public:
    CommonIterator()
    requires(concepts::DefaultInitializable<Iter>)
    = default;

    constexpr CommonIterator(Iter it) : m_state(c_<0ZU>, util::move(it)) {}
    constexpr CommonIterator(Sent sent) : m_state(c_<1ZU>, util::move(sent)) {}

    template<typename It, typename St>
    requires(concepts::ConvertibleTo<It const&, Iter> && concepts::ConvertibleTo<St const&, Sent>)
    constexpr CommonIterator(CommonIterator<It, St> const& other) : m_state(other.m_state) {}

    template<typename It, typename St>
    requires(concepts::ConvertibleTo<It const&, Iter> && concepts::ConvertibleTo<St const&, Sent>)
    constexpr auto operator=(CommonIterator<It, St> const& other) -> CommonIterator& {
        this->m_state = other.m_state;
        return *this;
    }

    constexpr auto operator*() const -> decltype(auto) { return *util::get<0>(m_state); }

    constexpr auto operator->() const
    requires((concepts::IndirectlyReadable<Iter const> && (requires(Iter const& it) { it.operator->(); })) ||
             concepts::Reference<meta::IteratorReference<Iter>> ||
             concepts::ConstructibleFrom<meta::IteratorValue<Iter>, meta::IteratorReference<Iter>>)
    {
        if constexpr (requires(Iter const& it) { it.operator->(); }) {
            return util::get<0>(m_state);
        } else if constexpr (concepts::Reference<meta::IteratorReference<Iter>>) {
            auto&& temp = *util::get<0>(m_state);
            return util::addressof(temp);
        } else {
            class Proxy {
            public:
                constexpr auto operator->() const -> meta::IteratorValue<Iter> const* {
                    return util::addressof(m_value);
                }

            private:
                constexpr Proxy(meta::IteratorReference<Iter>&& value) : m_value(util::move(value)) {}

                meta::IteratorValue<Iter> m_value;
            };

            return Proxy(*util::get<0>(m_state));
        }
    }

    constexpr void advance_one() { ++util::get<0>(m_state); }

private:
    template<concepts::InputIterator It, concepts::SentinelFor<It> St>
    requires(!concepts::SameAs<It, St> && concepts::Copyable<It>)
    friend class CommonIterator;

    template<typename It, concepts::SentinelFor<It> St>
    requires(concepts::SentinelFor<Sent, It>)
    constexpr friend auto operator==(CommonIterator const& a, CommonIterator<It, St> const& b) -> bool {
        if (a.m_state.index() == b.m_state.index()) {
            return true;
        }
        if (a.m_state.index() == 0) {
            return util::get<0>(a.m_state) == util::get<1>(b.m_state);
        }
        return util::get<1>(a.m_state) == util::get<0>(b.m_state);
    }

    template<typename It, concepts::SentinelFor<It> St>
    requires(concepts::SentinelFor<Sent, It> && concepts::EqualityComparableWith<Iter, It>)
    constexpr friend auto operator==(CommonIterator const& a, CommonIterator<It, St> const& b) -> bool {
        if (a.m_state.index() == 1 && b.m_state.index() == 1) {
            return true;
        }
        if (a.m_state.index() == 0 && b.m_state.index() == 0) {
            return util::get<0>(a.m_state) == util::get<0>(b.m_state);
        }
        if (a.m_state.index() == 0) {
            return util::get<0>(a.m_state) == util::get<1>(b.m_state);
        }
        return util::get<1>(a.m_state) == util::get<0>(b.m_state);
    }

    template<concepts::SizedSentinelFor<Iter> It, concepts::SizedSentinelFor<Iter> St>
    requires(concepts::SizedSentinelFor<Sent, Iter>)
    constexpr friend auto operator-(CommonIterator const& a, CommonIterator<It, St> const& b)
        -> meta::IteratorSSizeType<It> {
        if (a.m_state.index() == 1 && b.m_state.index() == 1) {
            return 0;
        }
        if (a.m_state.index() == 0 && b.m_state.index() == 0) {
            return util::get<0>(a.m_state) - util::get<0>(b.m_state);
        }
        if (a.m_state.index() == 0) {
            return util::get<0>(a.m_state) - util::get<1>(b.m_state);
        }
        return util::get<1>(a.m_state) - util::get<0>(b.m_state);
    }

    constexpr friend auto tag_invoke(types::Tag<iterator_move>, CommonIterator const& a) -> decltype(auto) {
        return iterator_move(util::get<0>(a.m_state));
    }

    template<concepts::IndirectlySwappable<Iter> It, typename St>
    constexpr friend void tag_invoke(types::Tag<iterator_swap>, CommonIterator const& a,
                                     CommonIterator<It, St> const& b) {
        return iterator_swap(util::get<0>(a.m_state), util::get<0>(b.m_state));
    }

    Variant<Iter, Sent> m_state;
};
}
