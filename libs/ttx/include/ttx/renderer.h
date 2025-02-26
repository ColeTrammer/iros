#pragma once

#include "di/container/string/string_view.h"
#include "di/io/vector_writer.h"
#include "dius/sync_file.h"
#include "dius/tty.h"
#include "ttx/cursor_style.h"
#include "ttx/graphics_rendition.h"

namespace ttx {
struct RenderedCursor {
    u32 cursor_row { 0 };
    u32 cursor_col { 0 };
    CursorStyle style { CursorStyle::SteadyBlock };
    bool hidden { false };
};

class Renderer {
public:
    void start(dius::tty::WindowSize size);
    auto finish(dius::SyncFile& output, RenderedCursor const& cursor) -> di::Result<>;

    void put_text(di::StringView text, u32 row, u32 col, GraphicsRendition const& graphics_rendition = {});
    void put_text(c32 text, u32 row, u32 col, GraphicsRendition const& graphics_rendition = {});

    void clear_row(u32 row, GraphicsRendition const& graphics_rendition = {});

    void set_bound(u32 row, u32 col, u32 width, u32 height);

private:
    di::VectorWriter<> m_buffer;
    dius::tty::WindowSize m_size;

    GraphicsRendition m_last_graphics_rendition;
    u32 m_last_cursor_row { 0 };
    u32 m_last_cursor_col { 0 };

    u32 m_row_offset { 0 };
    u32 m_col_offset { 0 };
    u32 m_bound_width { 0 };
    u32 m_bound_height { 0 };
};
}
