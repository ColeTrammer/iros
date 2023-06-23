#pragma once

#include <di/bit/bitset/prelude.h>
#include <di/bit/bitstruct/bit_tag.h>
#include <di/meta/algorithm.h>
#include <di/vocab/array/prelude.h>
#include <di/vocab/tuple/prelude.h>

namespace di::bit {
template<size_t byte_size, concepts::BitTag... Tags>
class [[gnu::packed]] BitStruct {
    using List = meta::List<Tags...>;

public:
    BitStruct() = default;

    template<concepts::BitTag... FromTags>
    requires(meta::Contains<List, FromTags> && ...)
    constexpr explicit BitStruct(FromTags... tags) {
        auto arguments = di::make_tuple(tags...);
        tuple_for_each(
            [&]<concepts::BitTag T>(T tag) {
                this->template set<T>(tag.get());
            },
            arguments);
    }

    template<concepts::BitTag Tag>
    constexpr void set(meta::BitValue<Tag> value) {
        Tag::value_into_bits(m_bitset, value);
    }

    template<concepts::BitTag Tag, typename R = meta::BitValue<Tag>>
    constexpr R get() const {
        return Tag::bits_into_value(m_bitset);
    }

private:
    BitSet<8 * byte_size> m_bitset;
};
}
