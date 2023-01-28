#include <cli/cli.h>
#include <ext/file.h>
#include <ext/mapped_file.h>
#include <ext/new_file.h>
#include <liim/container/hash_map.h>
#include <liim/container/path.h>
#include <liim/try.h>

struct PropertyRange {
    uint32_t start { 0 };
    uint32_t end_inclusive { 0 };
    uint32_t property_value { 0 };
};

struct PropertyRanges {
    NewVector<PropertyRange> ranges;
    LIIM::Container::HashMap<String, uint32_t> values;
};

namespace Ext {
template<>
struct ParserAdapter<PropertyRanges> {
    static Result<PropertyRanges, ParserError> parse(Parser& parser) {
        auto result = PropertyRanges {};
        auto& [ranges, values] = result;
        while (!parser.at_eof()) {
            auto line = parser.consume_line();
            if (!line.empty() && isxdigit(line[0])) {
                auto line_parser = Ext::Parser(line);
                auto start_of_range = TRY(Ext::parse_partial<Ext::HexNumber>(line_parser)).value;
                auto end_of_range = start_of_range;
                if (line_parser.try_consume("..")) {
                    end_of_range = TRY(Ext::parse_partial<Ext::HexNumber>(line_parser)).value;
                }
                line_parser.consume_matching(" ");
                TRY(line_parser.consume(";"));
                line_parser.consume_matching(" ");
                auto value = line_parser.consume_matching("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
                values.insert(Tuple { value, static_cast<uint32_t>(values.size()) });
                auto property_value = values[value];
                if (!ranges.empty() && ranges.back().end_inclusive + 1 == start_of_range &&
                    ranges.back().property_value == property_value) {
                    ranges.back().end_inclusive = end_of_range;
                } else {
                    ranges.push_back({ start_of_range, end_of_range, property_value });
                }
            }
        }
        return result;
    }
};
}

static Result<PropertyRanges, Ext::ParserError> parse_buffer(Span<const uint8_t> input) {
    return Ext::parse<PropertyRanges>(StringView(reinterpret_cast<const char*>(input.data()), input.size()));
}

struct Arguments {
    PathView unicode_data_directory { StringView(IROS_ROOT) };
    PathView output_directory;
};

constexpr static auto parser = [] {
    return Cli::Parser::of<Arguments>()
        .flag(Cli::Flag::defaulted<&Arguments::unicode_data_directory>()
                  .short_name('d')
                  .long_name("data-dir")
                  .description("Unicode data directory"))
        .flag(Cli::Flag::required<&Arguments::output_directory>().short_name('o').long_name("output").description("Output directory"));
}();

static Result<void, Error<>> generate_property_lookup(const Arguments& arguments, PathView data_file, StringView title_case_name,
                                                      StringView snake_case_name, StringView default_value) {
    auto db_file = TRY(Ext::NewFile::create(arguments.unicode_data_directory.join(data_file), Ext::NewFile::OpenMode::Readonly));
    auto buffer = TRY(db_file.map());
    auto result = TRY(parse_buffer(buffer.span()));

    Alg::sort(result.ranges, CompareThreeWay {}, &PropertyRange::start);

    {
        auto data_header =
            TRY(Ext::NewFile::create(arguments.output_directory.join("{}_data.h", snake_case_name), Ext::NewFile::OpenMode::WriteClobber));

        auto data_header_contents = String {};
        data_header_contents += "#pragma once\n\n";
        data_header_contents += "#include <liim/container/array.h>\n";
        data_header_contents += "#include <unicode/property_range.h>\n\n";
        data_header_contents += format("namespace Unicode::{}Data {{\n", title_case_name);
        data_header_contents += "constexpr static Array data = {\n";
        for (auto& range : result.ranges) {
            data_header_contents +=
                format("    Unicode::Detail::PropertyRange {{ {}, {}, {} }},\n", range.start, range.end_inclusive, range.property_value);
        }
        data_header_contents += "};\n";
        data_header_contents += "}\n";
        TRY(data_header.write(data_header_contents.view()));
    }

    {
        auto header =
            TRY(Ext::NewFile::create(arguments.output_directory.join("{}.h", snake_case_name), Ext::NewFile::OpenMode::WriteClobber));

        auto header_contents = String {};
        header_contents += "#pragma once\n\n";
        header_contents += "#include <stdint.h>\n\n";
        header_contents += "namespace Unicode {\n";
        header_contents += format("enum class {} {{\n", title_case_name);
        for (auto name : result.values.keys()) {
            header_contents += format("    {},\n", name);
        }
        header_contents += "};\n\n";
        header_contents += format("{} {}(uint32_t code_point);\n", title_case_name, snake_case_name);
        header_contents += "}\n";

        TRY(header.write(header_contents.view()));
    }

    {
        auto impl =
            TRY(Ext::NewFile::create(arguments.output_directory.join("{}.cpp", snake_case_name), Ext::NewFile::OpenMode::WriteClobber));

        auto impl_contents = String {};
        impl_contents += format("#include <unicode/{}.h>\n", snake_case_name);
        impl_contents += format("#include <unicode/{}_data.h>\n\n", snake_case_name);
        impl_contents += "namespace Unicode {\n";
        impl_contents += format("{} {}(uint32_t code_point) {{\n", title_case_name, snake_case_name);
        impl_contents += format("    auto result = Detail::find_range_for_code_point({}Data::data.span(), code_point);\n", title_case_name);
        impl_contents += format("    return static_cast<{}>(result.value_or(to_underlying({}::{})));\n", title_case_name, title_case_name,
                                default_value);
        impl_contents += "}\n";
        impl_contents += "}\n";

        TRY(impl.write(impl_contents.view()));
    }

    return {};
}

static Result<void, Error<>> generate_east_asian_width(const Arguments& arguments) {
    // The default value for the EastAsianWidth property is "N", meaning neutral.
    return generate_property_lookup(arguments, "extracted/DerivedEastAsianWidth.txt"_pv, "EastAsianWidth"sv, "east_asian_width"sv, "N"sv);
}

static Result<void, Error<>> unicode_main(Arguments arguments) {
    return generate_east_asian_width(arguments);
}

CLI_MAIN(unicode_main, parser);
