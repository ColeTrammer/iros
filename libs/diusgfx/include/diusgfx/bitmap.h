#pragma once

#include <di/assert/assert_bool.h>
#include <di/meta/util.h>
#include <di/types/byte.h>
#include <di/types/integers.h>
#include <di/vocab/bytes/byte_buffer.h>
#include <di/vocab/md/extents.h>
#include <di/vocab/md/extents_forward_declaration.h>
#include <di/vocab/md/mdspan.h>
#include <di/vocab/span/prelude.h>
#include <di/vocab/span/span_forward_declaration.h>

namespace gfx {
struct ARGBPixel {
    u8 blue;
    u8 green;
    u8 red;
    u8 alpha;
};

namespace bitmap {
    template<typename Buffer>
    class BitMapImpl {
    private:
        constexpr static auto is_const =
            di::SameAs<di::Span<byte const>, decltype(di::declval<Buffer const&>().span())>;

        using ConstBuffer = typename Buffer::ByteBuffer;

    public:
        BitMapImpl() = default;

        constexpr explicit BitMapImpl(Buffer buffer, usize width, usize height)
            : m_buffer(di::move(buffer)), m_width(width), m_height(height) {
            DI_ASSERT(byte_count() == width * height * sizeof(ARGBPixel));
        }

        constexpr auto width() const -> usize { return m_width; }
        constexpr auto height() const -> usize { return m_height; }
        constexpr auto byte_count() const -> usize { return as_raw_bytes().size(); };

        auto argb_pixels() const {
            return di::MDSpan { reinterpret_cast<di::meta::MaybeConst<is_const, ARGBPixel>*>(as_raw_bytes().data()),
                                m_height, m_width };
        }

        constexpr auto as_raw_bytes() const { return m_buffer.span(); }

    operator ConstBuffer() && requires(!is_const) { return ConstBuffer(di::move(m_buffer), m_width, m_height); }

        private : Buffer m_buffer;
        usize m_width;
        usize m_height;
    };
}

using BitMap = bitmap::BitMapImpl<di::ByteBuffer>;
using ExclusiveBitMap = bitmap::BitMapImpl<di::ExclusiveByteBuffer>;
}
