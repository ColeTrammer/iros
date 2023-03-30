#pragma once

#include <di/format/format_arg.h>
#include <di/format/format_args_storage.h>
#include <di/vocab/tuple/prelude.h>

namespace di::format {
template<concepts::FormatContext Context, concepts::Formattable... Types>
constexpr auto make_format_args(Types&&... values) {
    using Arg = FormatArg<Context>;
    return [&]<size_t... indices>(meta::IndexSequence<indices...>, [[maybe_unused]] Tuple<Types&...> values) {
        return FormatArgsStorage<sizeof...(Types), Arg>(Arg {
            in_place_type<meta::Conditional<meta::Contains<meta::VariantTypes<Arg>, Types>, Types, ErasedArg<Context>>>,
            util::get<indices>(values) }...);
    }(meta::IndexSequenceFor<Types...> {}, tie(values...));
}
}
