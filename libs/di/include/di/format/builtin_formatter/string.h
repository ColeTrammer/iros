#pragma once

#include "di/container/path/path_impl.h"
#include "di/container/path/path_view_impl.h"
#include "di/container/string/constant_string.h"
#include "di/container/string/mutable_string.h"
#include "di/container/string/string_view_impl.h"
#include "di/format/builtin_formatter/base.h"
#include "di/format/formatter.h"

namespace di::format {
template<concepts::detail::ConstantString T, concepts::Encoding Enc>
constexpr auto tag_invoke(types::Tag<formatter_in_place>, InPlaceType<T>, FormatParseContext<Enc>& parse_context,
                          bool debug) {
    return parse<detail::StringFormat>(parse_context.current_format_string()) % [=](detail::StringFormat format) {
        return [=](concepts::FormatContext auto& context, T const& value) -> Result<void> {
            auto width = format.width.transform(&detail::Width::value);
            auto precision = format.precision.transform(&detail::Precision::value);
            auto do_debug = format.type.has_value() ? format.type == detail::StringType::Debug : debug;
            return detail::present_string_view_to(context, format.fill_and_align, width, precision, do_debug,
                                                  value.view());
        };
    };
}

template<concepts::Encoding Enc, concepts::Encoding OtherEnc>
constexpr auto tag_invoke(types::Tag<formatter_in_place>, InPlaceType<container::PathViewImpl<Enc>>,
                          FormatParseContext<OtherEnc>& parse_context) {
    return di::format::formatter<container::string::StringViewImpl<Enc>, OtherEnc>(parse_context, true) %
           [](concepts::CopyConstructible auto formatter) {
               return [=](concepts::FormatContext auto& context, container::PathViewImpl<Enc> a) {
                   return formatter(context, a.data());
               };
           };
}

template<concepts::detail::MutableString T, concepts::Encoding Enc>
constexpr auto tag_invoke(types::Tag<formatter_in_place>, InPlaceType<container::PathImpl<T>>,
                          FormatParseContext<Enc>& parse_context) {
    return format::formatter<container::string::StringViewImpl<meta::Encoding<T>>, Enc>(parse_context, true) %
           [](concepts::CopyConstructible auto formatter) {
               return [=](concepts::FormatContext auto& context, container::PathImpl<T> const& a) {
                   return formatter(context, a.data());
               };
           };
}
}
