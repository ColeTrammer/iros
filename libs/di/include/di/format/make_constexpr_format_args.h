#pragma once

#include <di/format/format_arg.h>
#include <di/format/format_args_storage.h>
#include <di/vocab/tuple/prelude.h>

namespace di::format {
template<concepts::Formattable... Types>
constexpr auto make_constexpr_format_args(Types&&... values) {
    // Convert all types from rvalues to lvalues. This is ok because
    // the format args will be valid for the duration of a formatting
    // operation.
    using Arg = ConstexprFormatArg<Types&...>;
    return [&]<size_t... indices>(meta::IndexSequence<indices...>, [[maybe_unused]] Tuple<Types&...> values) {
        return FormatArgsStorage<sizeof...(Types), Arg>(Arg { in_place_index<indices>, util::get<indices>(values) }...);
    }
    (meta::IndexSequenceFor<Types...> {}, tie(values...));
}
}