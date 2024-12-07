#pragma once

#include "di/container/algorithm/min.h"
#include "di/container/concepts/prelude.h"
#include "di/container/iterator/prelude.h"
#include "di/container/meta/prelude.h"
#include "di/random/concepts/uniform_random_bit_generator.h"
#include "di/random/distribution/uniform_int_distribution.h"

namespace di::container {
namespace detail {
    struct SampleFunction {
        template<concepts::InputIterator It, concepts::SentinelFor<It> Sent, concepts::WeaklyIncrementable Out,
                 typename Gen, typename SSizeType = meta::IteratorSSizeType<It>>
        requires((concepts::ForwardIterator<It> || concepts::RandomAccessIterator<Out>) &&
                 concepts::IndirectlyCopyable<It, Out> &&
                 concepts::UniformRandomBitGenerator<meta::RemoveReference<Gen>>)
        constexpr auto operator()(It first, Sent last, Out out, meta::TypeIdentity<SSizeType> n, Gen&& generator) const
            -> Out {
            using Distribution = random::UniformIntDistribution<SSizeType>;
            using Param = Distribution::Param;

            auto distribution = Distribution {};

            if constexpr (concepts::ForwardIterator<It>) {
                // Implement selection sampling.

                auto count = container::distance(first, last);
                for (auto to_sample = container::min(count, n); to_sample != 0; --first) {
                    // Generate a random number between [0, count).
                    // Only keep the current element if the random number
                    // chosen is less than the number of samples which have
                    // not yet been generated.
                    if (distribution(generator, Param(0, --count)) < to_sample) {
                        *out = *first;
                        ++out;
                        --to_sample;
                    }
                }
                return out;
            } else {
                // Implement reservoir sampling, storing extra elements in the output iterator's storage.
                // This is Algorithm R, as described on wikipedia: https://en.wikipedia.org/wiki/Reservoir_sampling.

                // Copy the first n elements into the reservoir. If the input size is less than n,
                // we will have no more work to do.
                SSizeType reservoir_size = 0;
                for (; first != last && reservoir_size < n; ++first, ++reservoir_size) {
                    out[reservoir_size] = *first;
                }

                // Overwrite the reservoir with new elements if they get selected by the algorithm.
                for (auto size_so_far = reservoir_size; first != last; ++first, ++size_so_far) {
                    auto chosen_index = distribution(generator, Param(0, size_so_far));
                    if (chosen_index < n) {
                        out[chosen_index] = *first;
                    }
                }

                return out;
            }
        }

        template<concepts::InputContainer Con, concepts::WeaklyIncrementable Out, typename Gen>
        requires((concepts::ForwardContainer<Con> || concepts::RandomAccessIterator<Out>) &&
                 concepts::IndirectlyCopyable<meta::ContainerIterator<Con>, Out> &&
                 concepts::UniformRandomBitGenerator<meta::RemoveReference<Gen>>)
        constexpr auto operator()(Con&& container, Out out, meta::ContainerSSizeType<Con> n, Gen&& generator) const
            -> Out {
            return (*this)(container::begin(container), container::end(container), util::move(out), n, generator);
        }
    };
}

constexpr inline auto sample = detail::SampleFunction {};
}
