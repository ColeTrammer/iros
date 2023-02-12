#pragma once

#include <di/container/string/string_view_impl.h>
#include <di/format/concepts/format_context.h>
#include <di/format/concepts/formattable.h>
#include <di/util/voidify.h>
#include <di/vocab/variant/prelude.h>

namespace di::format {
template<concepts::Formattable... Types>
using ConstexprFormatArg = Variant<Types&..., Void>;

template<concepts::FormatContext Context>
class ErasedArg {
private:
    using Encoding = meta::Encoding<Context>;

public:
    template<concepts::Formattable T>
    constexpr explicit ErasedArg(T&& value)
        : m_pointer(util::voidify(util::addressof(value)))
        , m_do_format([](void* value, FormatParseContext<Encoding>& parse_context, Context& context,
                         bool debug) -> Result<void> {
            auto formatter = DI_TRY(format::formatter<T, Encoding>(parse_context, debug));
            return formatter(context, *static_cast<meta::RemoveCVRef<T>*>(value));
        }) {}

    Result<void> do_format(FormatParseContext<Encoding>& parse_context, Context& context, bool debug) {
        return m_do_format(m_pointer, parse_context, context, debug);
    }

private:
    void* m_pointer;
    Result<void> (*m_do_format)(void*, FormatParseContext<Encoding>&, Context&, bool debug);
};

template<concepts::FormatContext Context>
using FormatArg = Variant<Void, bool, meta::EncodingCodePoint<meta::Encoding<Context>>, int, unsigned int,
                          long long int, unsigned long long int, /* float, double, long double, */
                          container::string::StringViewImpl<meta::Encoding<Context>>, void const*, ErasedArg<Context>>;
}