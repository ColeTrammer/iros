#pragma once

#include <di/types/prelude.h>
#include <di/vocab/error/prelude.h>
#include <di/vocab/span/prelude.h>

namespace di::concepts {
template<typename T>
concept Reader = requires(T& reader, vocab::Span<Byte> buffer) {
                     { reader.read_some(buffer) } -> SameAs<Result<size_t>>;
                 };
}