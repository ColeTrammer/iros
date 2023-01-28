#pragma once

#include <graphics/ttf/forward.h>
#include <liim/forward.h>
#include <liim/option.h>

namespace TTF {
class GlyphMapping {
public:
    static UniquePtr<GlyphMapping> try_create(const ByteBuffer& buffer, const TableRecord& cmap_table_record);

    virtual ~GlyphMapping() {}

    // FIXME: support unicode variation sequenecs
    virtual Option<uint16_t> lookup_code_point(uint32_t code_point) = 0;

protected:
    GlyphMapping() = default;
};

class GlyphMappingFormat4 : public GlyphMapping {
public:
    static UniquePtr<GlyphMappingFormat4> try_create(const ByteBuffer& buffer, const TableRecord& cmap_table_record,
                                                     const EncodingRecord& encoding_record);

    explicit GlyphMappingFormat4(const CmapSubtable4& table) : m_table(table) {}
    virtual ~GlyphMappingFormat4() override {}

    virtual Option<uint16_t> lookup_code_point(uint32_t code_point) override;

    const CmapSubtable4& table() const { return m_table; }

private:
    const CmapSubtable4& m_table;
};
}
