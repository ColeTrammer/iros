#pragma once

#include <di/assert/prelude.h>
#include <di/container/string/string_view_impl.h>
#include <di/format/concepts/formattable.h>
#include <di/format/format_parse_context.h>
#include <di/meta/type_identity.h>

namespace di::format {
namespace detail {
    template<concepts::Encoding Enc, concepts::Formattable... Args>
    class FormatStringImpl {
    private:
        using StringView = container::string::StringViewImpl<Enc>;

    public:
        consteval FormatStringImpl(StringView view) : m_view(view) {
            auto parse_context = format::FormatParseContext<Enc> { view, sizeof...(Args) };
            for (auto part : parse_context) {
                if (!part) {
                    util::compile_time_fail<FixedString { "Invalid format string." }>();
                } else if (part->index() == 0) {
                    continue;
                }

                auto arg_index = util::get<1>(*part).index;
                function::index_dispatch<void, sizeof...(Args)>(arg_index, [&]<size_t index>(InPlaceIndex<index>) {
                    auto formatter = format::formatter<meta::At<meta::List<Args...>, index>>(parse_context);
                    if (!formatter) {
                        util::compile_time_fail<FixedString { "Invalid format string argument format." }>();
                    }
                });
            }
        }

        constexpr operator StringView() const { return m_view; }

        constexpr auto encoding() const { return m_view.encoding(); }

    private:
        StringView m_view;
    };
}

template<concepts::Encoding Enc, concepts::Formattable... Args>
using FormatStringImpl = detail::FormatStringImpl<Enc, meta::TypeIdentity<Args>...>;
}
