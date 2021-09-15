#pragma once

#include <liim/endian.h>
#include <liim/string_view.h>

namespace TTF {
struct [[gnu::packed]] Fixed {
    BigEndian<uint16_t> integer;
    BigEndian<uint16_t> decimal;
};

struct [[gnu::packed]] Tag {
    uint8_t data[4];
};

static inline StringView tag_to_string_view(const Tag& tag) {
    return StringView { reinterpret_cast<const char*>(&tag.data[0]), 4 };
}

struct [[gnu::packed]] Version16Dot16 {
    BigEndian<uint16_t> major;
    BigEndian<uint16_t> minor;
};

struct [[gnu::packed]] EncodingRecord {
    BigEndian<uint16_t> platform_id;
    BigEndian<uint16_t> platform_specific_id;
    BigEndian<uint32_t> offset;
};

struct [[gnu::packed]] Loca16Table {
    BigEndian<uint16_t> offsets16[0];
};

struct [[gnu::packed]] Loca32Table {
    BigEndian<uint32_t> offsets32[0];
};

struct [[gnu::packed]] LongHorizontalMetric {
    BigEndian<uint16_t> advance_width;
    BigEndian<int16_t> lsb;
};

struct [[gnu::packed]] HmtxTable {
    uint8_t metric_data[0];

    const LongHorizontalMetric* as_long_metrics() const { return reinterpret_cast<const LongHorizontalMetric*>(metric_data); }
    const BigEndian<int16_t>* as_short_metrics() const { return reinterpret_cast<const BigEndian<int16_t>*>(metric_data); }
};

struct [[gnu::packed]] CmapTable {
    BigEndian<uint16_t> version;
    BigEndian<uint16_t> num_tables;
    EncodingRecord encoding_records[];
};

struct [[gnu::packed]] MaxpTable {
    Version16Dot16 version;
    BigEndian<uint16_t> num_glyphs;
    BigEndian<uint16_t> max_points;
    BigEndian<uint16_t> max_contours;
    BigEndian<uint16_t> max_composite_points;
    BigEndian<uint16_t> max_composite_contours;
    BigEndian<uint16_t> max_zones;
    BigEndian<uint16_t> max_twilight_points;
    BigEndian<uint16_t> max_storage;
    BigEndian<uint16_t> max_function_defs;
    BigEndian<uint16_t> max_instruction_defs;
    BigEndian<uint16_t> max_size_of_instructions;
    BigEndian<uint16_t> max_component_elements;
    BigEndian<uint16_t> max_component_depth;
};

struct [[gnu::packed]] GlyphHeader {
    BigEndian<int16_t> number_of_contours;
    BigEndian<int16_t> x_min;
    BigEndian<int16_t> y_min;
    BigEndian<int16_t> x_max;
    BigEndian<int16_t> y_max;
};

struct [[gnu::packed]] GlyfTable {
    uint8_t glyph_data[0];
};

struct [[gnu::packed]] HeadTable {
    BigEndian<uint16_t> major_version;
    BigEndian<uint16_t> minor_version;
    Fixed font_revision;
    BigEndian<uint32_t> checksum_adjustment;
    BigEndian<uint32_t> magic_number;
    BigEndian<uint16_t> flags;
    BigEndian<uint16_t> units_per_em;
    BigEndian<uint64_t> created;
    BigEndian<uint64_t> modified;
    BigEndian<int16_t> x_min;
    BigEndian<int16_t> y_min;
    BigEndian<int16_t> x_max;
    BigEndian<int16_t> y_max;
    BigEndian<uint16_t> mac_style;
    BigEndian<uint16_t> lowest_rec_ppem;
    BigEndian<int16_t> font_direction_hint;
    BigEndian<int16_t> index_to_loc_format;
    BigEndian<int16_t> glyph_data_format;
};

struct [[gnu::packed]] HheaTable {
    BigEndian<uint16_t> major_version;
    BigEndian<uint16_t> minor_version;
    BigEndian<int16_t> ascender;
    BigEndian<int16_t> descender;
    BigEndian<int16_t> line_gap;
    BigEndian<uint16_t> advance_width_max;
    BigEndian<int16_t> min_left_side_bearing;
    BigEndian<int16_t> min_right_side_bearing;
    BigEndian<int16_t> x_max_extent;
    BigEndian<int16_t> caret_slope_rise;
    BigEndian<int16_t> caret_slope_run;
    BigEndian<int16_t> caret_offset;
    BigEndian<int16_t> reserved0;
    BigEndian<int16_t> reserved1;
    BigEndian<int16_t> reserved2;
    BigEndian<int16_t> reserved3;
    BigEndian<int16_t> metric_data_format;
    BigEndian<uint16_t> number_of_h_metrics;
};

struct [[gnu::packed]] TableRecord {
    Tag table_tag;
    BigEndian<uint32_t> checksum;
    BigEndian<uint32_t> offset;
    BigEndian<uint32_t> length;
};

struct [[gnu::packed]] TableDirectory {
    BigEndian<uint32_t> sfnt_version;
    BigEndian<uint16_t> num_tables;
    BigEndian<uint16_t> search_range;
    BigEndian<uint16_t> entry_selector;
    BigEndian<uint16_t> range_shift;
    TableRecord table_records[];
};
}
