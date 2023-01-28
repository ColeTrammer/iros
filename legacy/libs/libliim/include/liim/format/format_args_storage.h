#pragma once

#include <liim/format/format_args.h>
#include <liim/type_list.h>

namespace LIIM::Format {
namespace Detail {
    template<typename T>
    void do_format(const void* value, FormatContext& context, FormatParseContext& parse_context) {
        auto formatter = Formatter<T> {};
        formatter.parse(parse_context);
        formatter.format(*static_cast<const T*>(value), context);
    }
}

template<typename... Types>
class FormatArgsStorage final : public FormatArgs {
public:
    static constexpr int arg_count = TypeList::Count<Types...>::value;

    explicit FormatArgsStorage(const Types&... args)
        : FormatArgs { Span<FormatArg> { m_args, arg_count } }, m_args { FormatArg { &args, Detail::do_format<Types> }... } {}

private:
    FormatArg m_args[arg_count];
};

template<typename... Types>
FormatArgsStorage<Types...> make_format_args(const Types&... args) {
    return FormatArgsStorage { args... };
}
}
