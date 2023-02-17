#pragma once

#include <di/container/algorithm/equal.h>
#include <di/container/algorithm/find.h>
#include <di/container/algorithm/find_if_not.h>
#include <di/container/concepts/prelude.h>
#include <di/container/interface/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/container/string/string_view_impl.h>
#include <di/container/types/prelude.h>
#include <di/container/view/single.h>

namespace di::container {
template<concepts::Encoding Enc>
class PathIterator
    : public IteratorBase<PathIterator<Enc>, BidirectionalIteratorTag, string::StringViewImpl<Enc>, ssize_t> {
private:
    using View = string::StringViewImpl<Enc>;
    using CodePoint = meta::EncodingCodePoint<Enc>;

    constexpr explicit PathIterator(View bounds, View current) : m_bounds(bounds), m_current(current) {}

public:
    PathIterator() = default;

    constexpr auto operator*() const { return m_current; }

    constexpr auto current_data() { return m_current.data(); }

    constexpr void advance_one() {
        do {
            DI_ASSERT_NOT_EQ(m_current.begin(), m_bounds.end());
            auto new_start = find_if_not(m_current.end(), m_bounds.end(), function::equal(CodePoint('/')));

            // No more components, now produce end iterator.
            if (new_start == m_bounds.end()) {
                m_current = { m_bounds.end(), m_bounds.end() };
                return;
            }

            // The new end will be the next '/' after the new start.
            auto new_end = find(new_start, m_bounds.end(), CodePoint('/'));
            m_current = { new_start, new_end };

            // Skip over '.' components.
        } while (container::equal(m_current, view::single(CodePoint('.'))));
    }

    constexpr void back_one() {
        do {
            DI_ASSERT_NOT_EQ(m_current.begin(), m_bounds.begin());
            auto new_end = container::prev(m_current.begin());
            while (new_end != m_bounds.begin() && *new_end == CodePoint('/')) {
                --new_end;
            }

            // No more components, the iterator should not produce a single '/'.
            if (new_end == m_bounds.begin()) {
                m_current = { m_bounds.begin(), container::next(new_end) };
                return;
            }

            // The new start will be the last non-slash character before the new end.
            auto new_start = new_end;
            while (new_start != m_bounds.begin() && *new_start != CodePoint('/')) {
                --new_start;
            }

            // If the new_start points to a '/', advance it. Otherwise, this is a relative path.
            if (*new_start == CodePoint('/')) {
                m_current = { container::next(new_start), container::next(new_end) };
            } else {
                m_current = { new_start, container::next(new_end) };
            }

            // Skip over '.' components.
        } while (m_current.begin() != m_bounds.begin() && container::equal(m_current, view::single(CodePoint('.'))));
    }

private:
    template<typename, concepts::Encoding>
    friend class ConstantPathInterface;

    constexpr friend bool operator==(PathIterator const& a, PathIterator const& b) {
        return a.m_current == b.m_current;
    }
    constexpr friend auto operator<=>(PathIterator const& a, PathIterator const& b) {
        return a.m_current.data() <=> b.m_current.data();
    }

    View m_bounds;
    View m_current;
};
}