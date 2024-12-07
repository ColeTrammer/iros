#include "di/container/algorithm/iota.h"
#include "di/container/vector/vector.h"
#include "di/vocab/array/array.h"
#include "di/vocab/bytes/byte_buffer.h"
#include "dius/test/prelude.h"

namespace vocab_byte_buffer {
constexpr static void basic() {
    auto backing_store = di::Array { 1_b, 2_b, 3_b };
    auto buffer = di::ByteBuffer(backing_store);

    ASSERT_EQ(buffer.span(), backing_store.span());

    auto slice = *buffer.slice(1, 1);
    ASSERT_EQ(slice.span(), *backing_store.subspan(1, 1));
}

constexpr static void exclusive() {
    auto backing_store = di::Vector<byte> {};
    backing_store.resize(10);

    auto buffer = di::ExclusiveByteBuffer(di::move(backing_store));
    auto data = buffer.span();

    ASSERT_EQ(buffer.size(), 10ZU);

    di::container::fill(buffer.span(), 5_b);

    auto shared = di::ByteBuffer(di::move(buffer));
    ASSERT_EQ(data, shared.span());

    // NOLINTNEXTLINE(bugprone-use-after-move)
    ASSERT(buffer.empty());
}

TESTC(vocab_byte_buffer, basic)
TESTC(vocab_byte_buffer, exclusive)
}
