#pragma once

#include <di/types/prelude.h>
#include <di/vocab/error/prelude.h>
#include <di/vocab/span/prelude.h>

namespace di::concepts {
template<typename T>
concept Writer = requires(T& writer, vocab::Span<Byte const> data) {
                     { writer.write_some(data) } -> SameAs<Result<size_t>>;
                     { writer.flush() } -> SameAs<Result<void>>;
                 };
}