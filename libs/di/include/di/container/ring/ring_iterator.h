#pragma once

#include "di/container/iterator/prelude.h"
#include "di/container/types/prelude.h"

namespace di::container {
template<typename T>
class RingIterator : public IteratorBase<RingIterator<T>, RandomAccessIteratorTag, T, isize> {
private:
    constexpr static bool is_const = concepts::Const<T>;

    template<typename>
    friend class RingIterator;

public:
    RingIterator() = default;

    constexpr explicit RingIterator(T* current, T* head, T* tail, T* begin, T* end, bool at_end)
        : m_current(current), m_head(head), m_tail(tail), m_begin(begin), m_end(end), m_at_end(at_end) {}

    RingIterator(RingIterator const&) = default;
    RingIterator(RingIterator&&) = default;

    constexpr RingIterator(RingIterator<meta::RemoveConst<T>> const& other)
    requires(is_const)
        : m_current(other.m_current)
        , m_head(other.m_head)
        , m_tail(other.m_tail)
        , m_begin(other.m_begin)
        , m_end(other.m_end)
        , m_at_end(other.m_at_end) {}

    auto operator=(RingIterator const&) -> RingIterator& = default;
    auto operator=(RingIterator&&) -> RingIterator& = default;

    constexpr auto unconst_unsafe() const
    requires(is_const)
    {
        return RingIterator<meta::RemoveConst<T>>(
            const_cast<meta::RemoveConst<T>*>(m_current), const_cast<meta::RemoveConst<T>*>(m_head),
            const_cast<meta::RemoveConst<T>*>(m_tail), const_cast<meta::RemoveConst<T>*>(m_begin),
            const_cast<meta::RemoveConst<T>*>(m_end), m_at_end);
    }

    constexpr auto operator*() const -> T& {
        DI_ASSERT(!m_at_end);
        return *m_current;
    }
    constexpr auto operator->() const -> T* {
        DI_ASSERT(!m_at_end);
        return m_current;
    }

    constexpr void advance_one() {
        DI_ASSERT(!m_at_end);
        if (m_current == m_end - 1) {
            m_current = m_begin;
        } else {
            ++m_current;
        }
        if (m_current == m_tail) {
            m_at_end = true;
        }
    }

    constexpr void back_one() {
        DI_ASSERT(m_current != m_head || m_at_end);
        if (m_current == m_begin) {
            m_current = m_end - 1;
        } else {
            --m_current;
        }
        m_at_end = false;
    }

    constexpr void advance_n(isize n) {
        if (n == 0) {
            return;
        }
        if (n > 0) {
            auto max_distance = distance_to_tail();
            DI_ASSERT(n <= max_distance);

            if (n == max_distance) {
                m_current = m_tail;
                m_at_end = true;
                return;
            }

            auto distance_to_end = m_end - m_current;
            if (n < distance_to_end) {
                m_current += n;
            } else {
                m_current = m_begin + (n - distance_to_end);
            }
            return;
        }

        DI_ASSERT(-n <= m_end - m_begin);
        m_at_end = false;
        auto distance_to_begin = m_current - m_begin;
        if (-n <= distance_to_begin) {
            m_current += n;
        } else {
            m_current = m_end + n + distance_to_begin;
        }
    }

private:
    constexpr friend auto operator==(RingIterator const& a, RingIterator const& b) -> bool {
        DI_ASSERT(a.m_head == b.m_head);
        DI_ASSERT(a.m_tail == b.m_tail);
        DI_ASSERT(a.m_begin == b.m_begin);
        DI_ASSERT(a.m_end == b.m_end);
        return a.m_at_end == b.m_at_end && a.m_current == b.m_current;
    }

    constexpr friend auto operator<=>(RingIterator const& a, RingIterator const& b) {
        DI_ASSERT(a.m_head == b.m_head);
        DI_ASSERT(a.m_tail == b.m_tail);
        DI_ASSERT(a.m_begin == b.m_begin);
        DI_ASSERT(a.m_end == b.m_end);

        // First compare the end state.
        if (a.m_at_end || b.m_at_end) {
            return a.m_at_end <=> b.m_at_end;
        }

        // If a and b are not on the same "side" of the ring, then early return.
        if (auto result = (a.m_current < a.m_head) <=> (b.m_current < b.m_head); result != 0) {
            return result;
        }

        // Since a and b are on the same "side" of the ring, just compare pointers.
        return a.m_current <=> b.m_current;
    }

    constexpr auto distance_to_tail() const -> isize {
        if (m_at_end) {
            return 0;
        }
        if (m_current < m_tail) {
            return m_tail - m_current;
        }
        return m_end - m_current + (m_tail - m_begin);
    }

    constexpr friend auto operator-(RingIterator const& a, RingIterator const& b) -> isize {
        DI_ASSERT(a.m_head == b.m_head);
        DI_ASSERT(a.m_tail == b.m_tail);
        DI_ASSERT(a.m_begin == b.m_begin);
        DI_ASSERT(a.m_end == b.m_end);

        return b.distance_to_tail() - a.distance_to_tail();
    }

    T* m_current { nullptr };
    T* m_head { nullptr };
    T* m_tail { nullptr };
    T* m_begin { nullptr };
    T* m_end { nullptr };
    bool m_at_end { true };
};
}
