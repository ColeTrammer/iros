#pragma once

#include <di/container/string/prelude.h>
#include <di/container/view/transform.h>
#include <di/io/read_all.h>

namespace di::io {
namespace detail {
    struct ReadToString {
        constexpr Result<String> operator()(Impl<Reader> auto& reader) const {
            auto buffer = DI_TRY(read_all(reader));

            // FIXME: consider using reinterpret_cast<> when not in constexpr context.
            return util::move(buffer) | view::transform([](auto byte) {
                       return static_cast<c8>(byte);
                   }) |
                   container::to<Vector>() | container::to<String>();
        }
    };
}

constexpr inline auto read_to_string = detail::ReadToString {};
}

namespace di {
using io::read_to_string;
}
