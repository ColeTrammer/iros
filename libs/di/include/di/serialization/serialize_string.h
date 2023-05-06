#pragma once

#include <di/container/string/prelude.h>
#include <di/io/prelude.h>
#include <di/serialization/serialize.h>
#include <di/vocab/expected/invoke_as_fallible.h>
#include <di/vocab/expected/try_infallible.h>

namespace di::serialization {
namespace detail {
    struct SerializeStringFunction {
        template<typename Format, typename T, typename... Args,
                 typename S = meta::Serializer<Format, StringWriter<>, Args...>,
                 typename R = meta::LikeExpected<meta::SerializeResult<S, T>, container::String>>
        constexpr R operator()(Format format, T&& value, Args&&... args) const {
            auto serializer = serialization::serializer(format, StringWriter<> {}, util::forward<Args>(args)...);
            DI_TRY(serialization::serialize(serializer, value));
            return util::move(serializer).writer().output();
        }
    };
}

constexpr inline auto serialize_string = detail::SerializeStringFunction {};
}

namespace di {
using serialization::serialize_string;
}
