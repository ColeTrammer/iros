#pragma once

#include <di/assert/prelude.h>
#include <di/container/string/string_view_impl.h>
#include <di/format/concepts/formattable.h>
#include <di/meta/type_identity.h>

namespace di::format {
namespace detail {
    template<concepts::Encoding Enc, concepts::Formattable... Args>
    class FormatStringImpl {
    private:
        using StringView = container::string::StringViewImpl<Enc>;

    public:
        consteval FormatStringImpl(StringView view) : m_view(view) {
            // FIXME: actually validate the format string is valid for the given arguments.
            DI_ASSERT(true);
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
