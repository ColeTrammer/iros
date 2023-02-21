#pragma once

#include <di/concepts/integer.h>
#include <di/math/numeric_limits.h>
#include <di/meta/common_type.h>
#include <di/meta/make_unsigned.h>
#include <di/random/concepts/uniform_random_bit_generator.h>

namespace di::random {
template<concepts::Integer T = int>
class UniformIntDistribution {
public:
    using Result = T;
    class Param {
    public:
        using Distribution = UniformIntDistribution;

        constexpr Param() : Param(0) {}
        constexpr explicit Param(T a, T b = math::NumericLimits<T>::max) : m_a(a), m_b(b) {}

        constexpr T a() const { return m_a; }
        constexpr T b() const { return m_b; }

    private:
        constexpr friend bool operator==(Param const& a, Param const& b) { return a.a() == b.a() && a.b() == b.b(); }

        T m_a { 0 };
        T m_b { 0 };
    };

    constexpr UniformIntDistribution() : UniformIntDistribution(0) {}
    constexpr explicit UniformIntDistribution(T a, T b = math::NumericLimits<T>::max) : m_param(a, b) {}
    constexpr explicit UniformIntDistribution(Param const& param) : m_param(param) {}

    constexpr void reset() {}

    template<typename Gen>
    requires(concepts::UniformRandomBitGenerator<meta::RemoveReference<Gen>>)
    constexpr T operator()(Gen&& generator) const {
        return (*this)(generator, param());
    }

    template<typename Gen>
    requires(concepts::UniformRandomBitGenerator<meta::RemoveReference<Gen>>)
    constexpr T operator()(Gen&& generator, Param const& param) const {
        using U = meta::CommonType<meta::MakeUnsigned<T>, typename meta::RemoveReference<Gen>::Result>;

        // TODO: adopt a more sophisticated approach for implementing this function
        //       which does not naively retry the number generation on out of bounds.
        //       See 2018 paper: Fast Random Integer Generation in an Interval
        //       DANIEL LEMIRE, Université du Québec (TELUQ), Canada
        constexpr U generated_min = meta::RemoveReference<Gen>::min();
        constexpr U generated_max = meta::RemoveReference<Gen>::max();
        constexpr U generated_range = generated_max - generated_min;

        // A general simplification of generating an integer in the range
        // [a, b] is to consider generating an integer from [0, b - a] instead,
        // making sure to add a back in the end. When sampling from the generating
        // range, all that matters is the size of its output range, and so generated
        // values are always normalized by subtracting the generator's minimum value.
        auto generate = [&] -> U {
            return generator() - generated_min;
        };

        U a = param.a();
        U b = param.b();
        U desired_range = b - a;

        if (generated_range == desired_range) {
            // Simply sample the generated range, since it has the same
            // size as the desired range.
            return a + generate();
        }

        if (generated_range > desired_range) {
            // Down sample the generated bits.
            // Imagine having a generated range [0, 15], and a desired range [0, 2].
            // To down sample the generated range, note that the desired range has 3
            // elements, and the generated range has 16. Therefore, generate a random value
            // and discard it if it has the value 15. Otherwise, dividing by 5 will give
            // a properly sampled integer.
            U const desired_size = desired_range + 1;
            U const scale_factor = generated_range / desired_size;
            U const out_of_bounds = desired_size * scale_factor;
            for (;;) {
                auto result = generate();
                if (result >= out_of_bounds) {
                    continue;
                }
                return a + result / scale_factor;
            }
        } else {
            // Up sample the generated bits.
            // Imagine having a generated range [0, 3], and a desired range [0, 12].
            // To up sample the generated range, realize that the 13 element space in
            // the desired range can be partitioned into 4 parts, one for each possible
            // outcome of the generated range. So, recusively generate a uniform integer in
            // the range [0, 3] and multiply that by 4 to get an index in { 0, 4, 8, 12 }.
            // Then add to this index a non-scaled generated value, to produce a uniform
            // integer in the range [0, 15]. And finally, discard the value if it is out of bounds.
            U const generated_size = U(generated_range + 1);
            for (;;) {
                U const base_index = generated_size * (*this)(generator, Param(0u, desired_range / generated_size));
                U const index = generate();
                U const result = base_index + index;

                // Try again result is out of bounds (this also checks if the computation overflowed).
                if (result > desired_range || result < base_index) {
                    continue;
                }
                return a + result;
            }
        }
    }

    constexpr T a() const { return m_param.a(); }
    constexpr T b() const { return m_param.b(); }

    constexpr Param param() const { return m_param; }
    constexpr void param(Param const& param) const { m_param = param; }

    constexpr T min() const { return a(); }
    constexpr T max() const { return b(); }

private:
    constexpr friend bool operator==(UniformIntDistribution const& a, UniformIntDistribution const& b) {
        return a.param() == b.param();
    }

    Param m_param {};
};
}
