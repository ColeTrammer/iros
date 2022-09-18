#include <di/concepts/copy_constructible.h>
#include <di/concepts/default_initializable.h>
#include <di/concepts/integer.h>
#include <di/concepts/movable.h>
#include <di/concepts/object.h>
#include <di/concepts/same_as.h>
#include <di/concepts/semiregular.h>
#include <di/container/iterator/iterator_category.h>
#include <di/container/iterator/iterator_ssize_type.h>
#include <di/container/iterator/iterator_value.h>
#include <di/container/iterator/unreachable_sentinel.h>
#include <di/container/types/random_access_iterator_tag.h>
#include <di/container/view/view_interface.h>
#include <di/math/to_unsigned.h>
#include <di/meta/conditional.h>
#include <di/meta/make_signed.h>
#include <di/meta/remove_cv.h>
#include <di/types/ssize_t.h>
#include <di/util/address_of.h>
#include <di/util/move.h>

namespace di::container {
template<concepts::Movable T, concepts::Semiregular Bound = UnreachableSentinel>
requires(concepts::Object<T> && concepts::SameAs<T, meta::RemoveCV<T>> &&
         (concepts::Integer<Bound> || concepts::SameAs<Bound, UnreachableSentinel>) )
class RepeatView : public ViewInterface<RepeatView<T, Bound>> {
private:
    constexpr static bool is_bounded = !concepts::SameAs<Bound, UnreachableSentinel>;

    using IndexType = meta::Conditional<is_bounded, Bound, types::ssize_t>;
    using SSizeType = meta::MakeSigned<IndexType>;

    class Iterator {
    public:
        constexpr Iterator() = default;

        constexpr explicit Iterator(T const* value, IndexType current = IndexType()) : m_value(value), m_current(current) {}

        constexpr T const& operator*() const { return *m_value; }

        constexpr Iterator& operator++() {
            ++m_current;
            return *this;
        }

        constexpr Iterator operator++(int) {
            auto temp = *this;
            ++*this;
            return temp;
        }

        constexpr Iterator& operator--() {
            --m_value;
            return *this;
        }

        constexpr Iterator operator--(int) {
            auto temp = *this;
            --*this;
            return temp;
        }

        constexpr Iterator& operator+=(SSizeType n) {
            m_value += n;
            return *this;
        }

        constexpr Iterator& operator-=(SSizeType n) {
            m_value += n;
            return *this;
        }

        constexpr T const& operator[](SSizeType n) const { return *(*this + n); }

    private:
        constexpr friend bool operator==(Iterator const& a, Iterator const& b) { return a.m_current == b.m_current; }

        constexpr friend auto operator<=>(Iterator const& a, Iterator const& b) { return a.m_current <=> b.m_current; }

        constexpr friend Iterator operator+(Iterator x, SSizeType n) {
            x += n;
            return x;
        }

        constexpr friend Iterator operator+(SSizeType n, Iterator x) {
            x += n;
            return x;
        }

        constexpr friend Iterator operator-(Iterator x, SSizeType n) {
            x -= n;
            return x;
        }

        constexpr friend SSizeType operator-(Iterator const& a, Iterator const& b) {
            return static_cast<SSizeType>(a.m_current) - static_cast<SSizeType>(b.m_current);
        }

        constexpr friend types::RandomAccessIteratorTag tag_invoke(types::Tag<iterator_category>, types::InPlaceType<Iterator>) {
            return types::RandomAccessIteratorTag {};
        }
        constexpr friend T tag_invoke(types::Tag<iterator_value>, types::InPlaceType<Iterator>) {}
        constexpr friend SSizeType tag_invoke(types::Tag<iterator_ssize_type>, types::InPlaceType<Iterator>) {}

        T const* m_value { nullptr };
        IndexType m_current {};
    };

public:
    constexpr RepeatView()
    requires(concepts::DefaultInitializable<T>)
    = default;

    constexpr explicit RepeatView(T const& value, Bound bound = Bound())
    requires(concepts::CopyConstructible<T>)
        : m_value(value), m_bound(bound) {}

    constexpr explicit RepeatView(T&& value, Bound bound = Bound()) : m_value(util::move(value)), m_bound(bound) {}

    constexpr Iterator begin() const { return Iterator(util::address_of(m_value)); }

    constexpr Iterator end() const { return Iterator(util::address_of(m_value), m_bound); }
    constexpr UnreachableSentinel end() const
    requires(!is_bounded)
    {
        return unreachable_sentinel;
    }

    constexpr auto size() const
    requires(is_bounded)
    {
        return math::to_unsigned(m_bound);
    }

private:
    T m_value;
    Bound m_bound;
};

template<typename T, typename Bound>
RepeatView(T, Bound) -> RepeatView<T, Bound>;
}
