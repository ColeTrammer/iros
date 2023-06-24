#pragma once

#include <di/container/string/string_impl.h>
#include <di/container/string/string_view_impl.h>
#include <di/format/vpresent_encoded_context.h>
#include <di/function/monad/monad_try.h>
#include <di/meta/vocab.h>
#include <di/util/move.h>

namespace di::format {
namespace detail {
    template<concepts::Encoding Enc>
    struct VPresentEncodedFunction {
        using View = container::string::StringViewImpl<Enc>;
        using Str = container::string::StringImpl<Enc>;

        template<concepts::FormatArg Arg>
        constexpr Result<Str> operator()(View format, FormatArgs<Arg> args) const {
            auto context = FormatContext<Enc> {};
            DI_TRY(vpresent_encoded_context<Enc>(format, util::move(args), context));
            return util::move(context).output();
        }
    };
}

template<concepts::Encoding Enc>
constexpr inline auto vpresent_encoded = detail::VPresentEncodedFunction<Enc> {};
}
