#pragma once

#include "di/assert/assert_bool.h"
#include "di/bit/endian/endian.h"
#include "di/bit/endian/prelude.h"
#include "di/container/algorithm/all_of.h"
#include "di/container/algorithm/compare.h"
#include "di/container/view/prelude.h"
#include "di/container/view/range.h"
#include "di/container/view/reverse.h"
#include "di/container/view/zip.h"
#include "di/function/equal.h"
#include "di/math/numeric_limits.h"
#include "di/types/prelude.h"
#include "di/vocab/span/prelude.h"

namespace di::math::bigint {
using StorageType = unsigned long;

template<usize words>
struct FixedOps {
    constexpr static auto word_index(usize bit) -> usize { return bit / (8 * sizeof(StorageType)); }

    constexpr static auto get_bit(Span<StorageType const, words> storage, usize bit) -> bool {
        auto word_index = FixedOps::word_index(bit);
        auto bit_index = bit % (8 * sizeof(StorageType));
        return (storage[word_index] & (StorageType(1) << bit_index)) != 0;
    }

    constexpr static void set_bit(Span<StorageType, words> storage, usize bit, bool value) {
        auto word_index = FixedOps::word_index(bit);
        auto bit_index = bit % (8 * sizeof(StorageType));
        if (value) {
            storage[word_index] |= (StorageType(1) << bit_index);
        } else {
            storage[word_index] &= ~(StorageType(1) << bit_index);
        }
    }

    constexpr static auto twos_complement_negative(Span<StorageType const, words> storage) -> bool {
        return get_bit(storage, words * 8 * sizeof(StorageType) - 1);
    }

    constexpr static auto compare(Span<StorageType const, words> lhs, Span<StorageType const, words> rhs)
        -> strong_ordering {
        return container::compare(container::view::reverse(lhs), container::view::reverse(rhs));
    }

    constexpr static void shift_left_one(Span<StorageType, words> value) {
        bool carry = false;
        for (auto& word : value) {
            auto new_carry = word & (StorageType(1) << (8 * sizeof(StorageType) - 1));
            word <<= 1;
            if (carry) {
                word |= 1;
            }
            carry = new_carry;
        }
    }

    constexpr static void add_one(Span<StorageType, words> value) {
        for (auto& word : value) {
            if (++word != 0) {
                break;
            }
        }
    }

    constexpr static void negate(Span<StorageType, words> value) {
        for (auto& word : value) {
            word = ~word;
        }
        add_one(value);
    }

    constexpr static void add(Span<StorageType, words> a, Span<StorageType const, words> b) {
        bool carry = false;
        for (auto [x, y] : container::view::zip(a, b)) {
            auto new_carry = x > (NumericLimits<StorageType>::max - y);
            x += y;
            if (carry) {
                new_carry |= ++x == 0;
            }
            carry = new_carry;
        }
    }

    constexpr static void subtract(Span<StorageType, words> a, Span<StorageType const, words> b) {
        auto b_negated = util::to_owned(b);
        negate(b_negated.span());
        add(a, b_negated);
    }

    constexpr static void div_mod(Span<StorageType const, words> dividend, Span<StorageType const, words> divisor,
                                  Span<StorageType, words> quotient, Span<StorageType, words> remainder) {
        // https://en.wikipedia.org/wiki/Division_algorithm#Integer_division_(unsigned)_with_remainder
        DI_ASSERT(!container::all_of(divisor, function::equal(StorageType(0))));

        constexpr auto bits = 8 * sizeof(StorageType) * words;
        for (auto i : container::view::range(bits) | container::view::reverse) {
            shift_left_one(remainder);
            set_bit(remainder, 0, get_bit(dividend, i));
            if (compare(remainder, divisor) >= 0) {
                subtract(remainder, divisor);
                set_bit(quotient, i, true);
            }
        }
    }
};
}
