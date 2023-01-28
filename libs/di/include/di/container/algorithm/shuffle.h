#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/random/concepts/uniform_random_bit_generator.h>
#include <di/random/distribution/uniform_int_distribution.h>

namespace di::container {
namespace detail {
    struct ShuffleFunction {
        template<concepts::RandomAccessIterator It, concepts::SentinelFor<It> Sent, typename Gen>
        requires(concepts::Permutable<It> && concepts::UniformRandomBitGenerator<meta::RemoveReference<Gen>>)
        constexpr It operator()(It first, Sent last, Gen&& generator) const {
            using SSizeType = meta::IteratorSSizeType<It>;
            using Distribution = random::UniformIntDistribution<SSizeType>;
            using Param = Distribution::Param;

            auto const size = container::distance(first, last);
            auto distribution = Distribution();

            // Fisher-Yates shuffle.
            for (SSizeType i = size - 1; i > 0; i--) {
                auto j = distribution(generator, Param { 0, i });
                container::iterator_swap(first + i, first + j);
            }
            return first + size;
        }

        template<concepts::RandomAccessContainer Con, typename Gen>
        requires(concepts::Permutable<meta::ContainerIterator<Con>> &&
                 concepts::UniformRandomBitGenerator<meta::RemoveReference<Gen>>)
        constexpr meta::BorrowedIterator<Con> operator()(Con&& container, Gen&& generator) const {
            return (*this)(container::begin(container), container::end(container), generator);
        }
    };
}

constexpr inline auto shuffle = detail::ShuffleFunction {};
}