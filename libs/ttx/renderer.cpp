#include "ttx/renderer.h"

#include "di/io/writer_print.h"
#include "di/meta/constexpr.h"
#include "dius/print.h"
#include "dius/sync_file.h"
#include "dius/tty.h"
#include "ttx/graphics_rendition.h"

namespace ttx {
void Renderer::start(dius::tty::WindowSize size) {
    m_buffer = {};
    m_size = size;
    m_row_offset = 0;
    m_col_offset = 0;
    m_bound_width = size.cols;
    m_bound_height = size.rows;

    // Reset the cursor here since that way we don't have to worry about how window size
    // changes should affect things.
    m_last_cursor_row = 0;
    m_last_cursor_col = 0;

    di::writer_print<di::String::Encoding>(m_buffer, "\033[?2026h"_sv);
    di::writer_print<di::String::Encoding>(m_buffer, "\033[?25l"_sv);
    di::writer_print<di::String::Encoding>(m_buffer, "\033[H"_sv);
}

auto Renderer::finish(dius::SyncFile& output, RenderedCursor const& cursor) -> di::Result<> {
    if (!cursor.hidden) {
        di::writer_print<di::String::Encoding>(m_buffer, "\033[{};{}H"_sv, cursor.cursor_row + 1,
                                               cursor.cursor_col + 1);
        di::writer_print<di::String::Encoding>(m_buffer, "\033[{} q"_sv, i32(cursor.style));
        di::writer_print<di::String::Encoding>(m_buffer, "\033[?25h"_sv);
    }

    di::writer_print<di::String::Encoding>(m_buffer, "\033[?2026l"_sv);

    auto text = di::move(m_buffer).vector();
    m_buffer = {};
    return output.write_exactly(di::as_bytes(text.span()));
}

void Renderer::put_text(di::StringView text, u32 row, u32 col, GraphicsRendition const& rendition) {
    if (col >= m_bound_width || row >= m_bound_height) {
        return;
    }

    // TODO: use an actual width measurement, using either grapheme glusters or east asian width.
    auto text_width = u32(di::distance(text));
    text_width = di::min(text_width, m_bound_width - col);

    auto truncated_text = di::take(text, text_width);

    row += m_row_offset;
    col += m_col_offset;

    if (m_last_cursor_row != row || m_last_cursor_col != col) {
        m_last_cursor_row = row;
        m_last_cursor_col = col;
        di::writer_print<di::String::Encoding>(m_buffer, "\033[{};{}H"_sv, row + 1, col + 1);
    }

    if (m_last_graphics_rendition != rendition) {
        m_last_graphics_rendition = rendition;

        for (auto& params : rendition.as_csi_params()) {
            di::writer_print<di::String::Encoding>(m_buffer, "\033[{}m"_sv, params);
        }
    }

    for (auto code_point : truncated_text) {
        di::writer_print<di::String::Encoding>(m_buffer, "{}"_sv, code_point);
        m_last_cursor_col++;
    }
}

void Renderer::put_text(c32 text, u32 row, u32 col, GraphicsRendition const& rendition) {
    auto string = di::container::string::StringImpl<di::String::Encoding, di::StaticVector<c8, di::Constexpr<4zu>>> {};
    (void) string.push_back(text);
    put_text(string.view(), row, col, rendition);
}

void Renderer::set_bound(u32 row, u32 col, u32 width, u32 height) {
    m_row_offset = row;
    m_col_offset = col;
    m_bound_width = width;
    m_bound_height = height;
}
}
