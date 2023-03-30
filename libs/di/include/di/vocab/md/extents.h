#pragma once

#include <di/concepts/disjunction.h>
#include <di/container/view/range.h>
#include <di/math/numeric_limits.h>
#include <di/math/to_unsigned.h>
#include <di/types/prelude.h>
#include <di/vocab/array/prelude.h>

namespace di::vocab {
template<concepts::Integer T, size_t... extents>
requires(
    concepts::Conjunction<(extents == dynamic_extent || extents <= math::to_unsigned(math::NumericLimits<T>::max))...>)
class Extents {
public:
    constexpr static size_t static_extent(size_t index) {
        auto result = Array { extents... };
        return result.data()[index];
    }

private:
    constexpr static size_t dynamic_index(size_t index) {
        auto result = Array<size_t, rank() + 1> {};
        size_t count = 0;
        for (auto i : container::view::range(1zu, rank() + 1)) {
            if (static_extent(i - 1) == dynamic_extent) {
                ++count;
            }
            result.data()[i] = count;
        }

        return result.data()[index];
    }

    constexpr static size_t dynamic_index_inv(size_t index) {
        constexpr auto result = [] {
            auto answer = Array<size_t, rank_dynamic()> {};
            for (auto i : container::view::range(rank())) {
                for (auto r : container::view::range(rank())) {
                    if (dynamic_index(r + 1) == i + 1) {
                        answer.data()[i] = r;
                        break;
                    }
                }
            }
            return answer;
        }();

        return result.data()[index];
    }

public:
    using SizeType = T;
    using RankType = size_t;

    constexpr static size_t rank() { return sizeof...(extents); }
    constexpr static size_t rank_dynamic() { return dynamic_index(rank()); }

    constexpr Extents() { m_dynamic_extents.fill(0); }

    template<typename OtherSizeType, size_t... other_extents>
    requires(sizeof...(other_extents) == rank() &&
             concepts::Conjunction<(other_extents == dynamic_extent || extents == dynamic_extent ||
                                    other_extents == extents)...>)
    constexpr explicit(concepts::Disjunction<((extents != dynamic_extent) && (other_extents == dynamic_extent))...> ||
                       math::to_unsigned(math::NumericLimits<SizeType>::max) <
                           math::to_unsigned(math::NumericLimits<OtherSizeType>::max))
        Extents(Extents<OtherSizeType, other_extents...> const& other) {
        for (auto i : container::view::range(rank())) {
            if (static_extent(i) == dynamic_extent) {
                m_dynamic_extents[dynamic_index(i)] = other.extent(i);
            } else {
                DI_ASSERT_EQ(this->extent(i), other.extent(i));
            }
        }
    }

    template<typename... OtherSizeType>
    requires(concepts::Conjunction<concepts::ConvertibleTo<OtherSizeType, SizeType>...> &&
             (sizeof...(OtherSizeType) == rank_dynamic() || sizeof...(OtherSizeType) == rank()))
    constexpr explicit Extents(OtherSizeType... values) {
        if constexpr (sizeof...(OtherSizeType) == rank_dynamic()) {
            DI_ASSERT(((values >= 0) && ...));
            DI_ASSERT(((math::to_unsigned(values) <= math::NumericLimits<SizeType>::max) && ...));
            m_dynamic_extents = { static_cast<SizeType>(util::move(values))... };
        } else {
            auto as_extents = Extents<SizeType, (values, dynamic_extent)...> { values... };
            auto new_extents = Extents(as_extents);
            *this = new_extents;
        }
    }

    template<typename OtherSizeType, size_t N>
    requires(concepts::ConvertibleTo<OtherSizeType const&, SizeType> && (N == rank_dynamic() || N == rank()))
    constexpr explicit(N != rank_dynamic()) Extents(Span<OtherSizeType, N> extents_array) {
        function::unpack<meta::MakeIndexSequence<rank_dynamic()>>(
            [&]<size_t... indices>(meta::IndexSequence<indices...>) {
                if constexpr (N == rank_dynamic()) {
                    m_dynamic_extents = { util::as_const(extents_array[indices])... };
                } else {
                    m_dynamic_extents = { util::as_const(extents_array[dynamic_index_inv(indices)])... };
                }
            });
    }

    template<typename OtherSizeType, size_t N>
    requires(concepts::ConvertibleTo<OtherSizeType const&, SizeType> && (N == rank_dynamic() || N == rank()))
    constexpr explicit(N != rank_dynamic()) Extents(Array<OtherSizeType, N> const& extents_array)
        : Extents(extents_array.span()) {}

    constexpr size_t extent(size_t index) const {
        auto extent = static_extent(index);
        if (extent != dynamic_extent) {
            return extent;
        } else if constexpr (rank_dynamic() != 0) {
            return m_dynamic_extents[dynamic_index(index)];
        } else {
            util::unreachable();
        }
    }

    constexpr size_t fwd_prod_of_extents(size_t i) const {
        size_t result = 1;
        for (auto i : container::view::range(i)) {
            result *= extent(i);
        }
        return result;
    }

    constexpr size_t rev_prod_of_extents(size_t i) const {
        size_t result = 1;
        for (auto i : container::view::range(i + 1, rank())) {
            result *= extent(i);
        }
        return result;
    }

    template<typename OtherSizeType>
    constexpr static auto index_cast(OtherSizeType&& i) {
        if constexpr (concepts::Integral<OtherSizeType> && !concepts::SameAs<OtherSizeType, bool>) {
            return i;
        } else {
            return static_cast<SizeType>(i);
        }
    }

private:
    template<typename OtherSizeType, size_t... other_extents>
    constexpr friend bool operator==(Extents const& a, Extents<OtherSizeType, other_extents...> const& b) {
        if (a.rank() != b.rank()) {
            return false;
        }
        for (auto i : container::view::range(rank())) {
            if (a.extent(i) != b.extent(i)) {
                return false;
            }
        }
        return true;
    }

    [[no_unique_address]] Array<SizeType, rank_dynamic()> m_dynamic_extents {};
};

template<typename... Integrals>
requires(concepts::Conjunction<concepts::ConvertibleTo<Integrals, size_t>...>)
explicit Extents(Integrals...) -> Extents<size_t, (Integrals {}, dynamic_extent)...>;
}
