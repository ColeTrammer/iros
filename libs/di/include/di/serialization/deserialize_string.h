#pragma once

#include <di/container/string/prelude.h>
#include <di/container/string/string_view.h>
#include <di/io/prelude.h>
#include <di/serialization/deserialize.h>
#include <di/vocab/expected/invoke_as_fallible.h>
#include <di/vocab/expected/try_infallible.h>

namespace di::serialization {
namespace detail {
    template<typename T>
    struct DeserializeStringFunction {
        template<typename Format, typename... Args,
                 typename D = meta::Deserializer<Format, StringReader<container::StringView>, Args...>,
                 typename R = meta::DeserializeResult<D, T>>
        requires(concepts::Deserializable<T, D>)
        constexpr auto operator()(Format format, container::StringView view, Args&&... args) const -> R {
            auto deserializer = serialization::deserializer(format, StringReader<container::StringView> { view },
                                                            util::forward<Args>(args)...);
            return serialization::deserialize<T>(deserializer);
        }
    };
}

template<typename T>
constexpr inline auto deserialize_string = detail::DeserializeStringFunction<meta::RemoveCVRef<T>> {};
}

namespace di {
using serialization::deserialize_string;
}
