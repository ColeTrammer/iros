#include <ext/mapped_file.h>
#include <graphics/ttf/binary_format.h>
#include <graphics/ttf/font.h>
#include <liim/byte_io.h>
#include <liim/endian.h>
#include <liim/format.h>
#include <sys/mman.h>

int main() {
    auto path = RESOURCE_ROOT "/usr/share/font.ttf";

    auto mapped_file = Ext::try_map_file(path, PROT_READ, MAP_SHARED);
    assert(mapped_file);

    auto font = TTF::Font::try_create_from_buffer(move(*mapped_file));
    assert(font);

    auto byte_reader = ByteReader { font->raw_buffer().span() };

    auto& table_directory = font->table_directory();
    debug_log("table directory:\n"
              "  version:        {:#.8X}\n"
              "  num_tables:     {}\n"
              "  search_range:   {}\n"
              "  entry_selector: {}\n"
              "  range_shift:    {}",
              table_directory.sfnt_version, table_directory.num_tables, table_directory.search_range, table_directory.entry_selector,
              table_directory.range_shift);
    for (uint32_t i = 0; i < table_directory.num_tables; i++) {
        auto& table = table_directory.table_records[i];
        debug_log("  table record:\n"
                  "    tag:        {}\n"
                  "    checksum:   {}\n"
                  "    offset:     {}\n"
                  "    length:     {}",
                  TTF::tag_to_string_view(table.table_tag), table.checksum, table.offset, table.length);
    }

    auto* cmap = font->find_table("cmap");
    assert(cmap);

    auto* cmap_header = byte_reader.pointer_at_offset<TTF::CmapTable>(cmap->offset);
    assert(cmap_header);
    // FIXME: validate cmap_header size.

    debug_log("cmap table:\n"
              "  version:    {}\n"
              "  num tables: {}",
              cmap_header->version, cmap_header->num_tables);

    for (uint16_t i = 0; i < cmap_header->num_tables; i++) {
        auto& encoding_table = cmap_header->encoding_records[i];
        debug_log("  encoding table:\n"
                  "    platform id:          {}\n"
                  "    platform specific id: {}\n"
                  "    offset:               {}\n"
                  "    format:               {}",
                  encoding_table.platform_id, encoding_table.platform_specific_id, encoding_table.offset,
                  *byte_reader.pointer_at_offset<BigEndian<uint16_t>>(cmap->offset + encoding_table.offset));
    }

    auto* head = font->find_table("head");
    assert(head);

    auto* head_table = byte_reader.pointer_at_offset<TTF::HeadTable>(head->offset);
    assert(head_table);

    debug_log("head table:\n"
              "  major version:       {}\n"
              "  minor version:       {}\n"
              "  font revision:       {}.{}\n"
              "  checksum adjustment: {}\n"
              "  magic number:        {:#.8X}\n"
              "  flags:               {:#.4X}\n"
              "  units per em:        {}\n"
              "  created:             {}\n"
              "  modified:            {}\n"
              "  x min:               {}\n"
              "  y min:               {}\n"
              "  x max:               {}\n"
              "  y max:               {}\n"
              "  mac style:           {:.8X}\n"
              "  lowest rec ppem:     {}\n"
              "  font direction hint: {}\n"
              "  index to loc format: {}\n"
              "  glyph data format:   {}",
              head_table->major_version, head_table->minor_version, head_table->font_revision.integer, head_table->font_revision.decimal,
              head_table->checksum_adjustment, head_table->magic_number, head_table->flags, head_table->units_per_em, head_table->created,
              head_table->modified, head_table->x_min, head_table->y_min, head_table->x_max, head_table->y_max, head_table->mac_style,
              head_table->lowest_rec_ppem, head_table->font_direction_hint, head_table->index_to_loc_format, head_table->glyph_data_format);

    auto* maxp = font->find_table("maxp");
    assert(maxp);

    auto* maxp_table = byte_reader.pointer_at_offset<TTF::MaxpTable>(maxp->offset);
    assert(maxp_table);

    debug_log("maxp table:\n"
              "  version:                  {}.{}\n"
              "  num glyphs:               {}\n"
              "  max points:               {}\n"
              "  max contours:             {}\n"
              "  max composite points:     {}\n"
              "  max composite contours:   {}\n"
              "  max zones:                {}\n"
              "  max twilight points:      {}\n"
              "  max storage:              {}\n"
              "  max function defs:        {}\n"
              "  max instruction defs:     {}\n"
              "  max stack elements:       {}\n"
              "  max size of instructions: {}\n"
              "  max component elements:   {}\n"
              "  max component depth:      {}",
              maxp_table->version.major, maxp_table->version.minor, maxp_table->num_glyphs, maxp_table->max_points,
              maxp_table->max_contours, maxp_table->max_composite_points, maxp_table->max_composite_contours, maxp_table->max_zones,
              maxp_table->max_twilight_points, maxp_table->max_storage, maxp_table->max_function_defs, maxp_table->max_instruction_defs,
              maxp_table->max_storage, maxp_table->max_size_of_instructions, maxp_table->max_component_elements,
              maxp_table->max_component_depth);

    auto* hhea = font->find_table("hhea");
    assert(hhea);

    auto* hhea_table = byte_reader.pointer_at_offset<TTF::HheaTable>(hhea->offset);
    assert(hhea_table);

    debug_log("hhea table:\n"
              "major_version: {}\n"
              "minor version: {}\n"
              "ascender: {}\n"
              "descender: {}\n"
              "line gap: {}\n"
              "advance_width_max: {}\n"
              "min left side bearing: {}\n"
              "min right side bearing: {}\n"
              "x max extent: {}\n"
              "caret slope rise: {}\n"
              "caret slope run: {}\n"
              "caret offset: {}\n"
              "metric data format: {}\n"
              "number of h metrics: {}",
              hhea_table->major_version, hhea_table->minor_version, hhea_table->ascender, hhea_table->descender, hhea_table->line_gap,
              hhea_table->advance_width_max, hhea_table->min_left_side_bearing, hhea_table->min_right_side_bearing,
              hhea_table->x_max_extent, hhea_table->caret_slope_rise, hhea_table->caret_slope_run, hhea_table->caret_offset,
              hhea_table->metric_data_format, hhea_table->number_of_h_metrics);
}
