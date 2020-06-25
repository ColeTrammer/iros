#include <assert.h>
#include <fcntl.h>
#include <graphics/renderer.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <window_server/window.h>

#include <kernel/hal/x86_64/drivers/vga.h>

#include "vga_buffer.h"

VgaBuffer::VgaBuffer(GraphicsContainer& wrapper) : m_graphics_container(wrapper) {}
VgaBuffer::~VgaBuffer() {}

#ifdef KERNEL_NO_GRAPHICS
void VgaBuffer::refresh() {}
#else
void VgaBuffer::restore_cursor_pixels() {
    if (m_cursor_was_drawn) {
        for (int r = 0; r < 16; r++) {
            for (int c = 0; c < 8; c++) {
                window().pixels()->put_pixel(c + m_drawn_cursor_col * 8, r + m_drawn_cursor_row * 16, m_cursor_location_save[r][c]);
            }
        }
    }

    m_cursor_location_save.clear();
    m_cursor_was_drawn = false;
}

void VgaBuffer::refresh() {
    restore_cursor_pixels();

    if (m_is_cursor_enabled) {
        m_cursor_was_drawn = true;
        m_drawn_cursor_row = m_cursor_row;
        m_drawn_cursor_col = m_cursor_col;

        for (int r = 0; r < 16; r++) {
            auto row = Vector<uint32_t>();
            for (int c = 0; c < 8; c++) {
                row.add(window().pixels()->get_pixel(c + m_cursor_col * 8, r + m_cursor_row * 16));
            }
            m_cursor_location_save.add(move(row));
        }

        Color bg = m_cursor_location_save[0][0];
        Color fg = bg.invert();
        for (int r = 0; r < m_cursor_location_save.size(); r++) {
            auto& row = m_cursor_location_save[r];
            for (int c = 0; c < row.size(); c++) {
                if (bg.color() != row[c]) {
                    fg = row[c];
                    goto break_out;
                }
            }
        }

    break_out:
        Renderer renderer(*window().pixels());
        for (int r = 0; r < 16; r++) {
            for (int c = 0; c < 8; c++) {
                if (bg == m_cursor_location_save[r][c]) {
                    window().pixels()->put_pixel(c + m_cursor_col * 8, r + m_cursor_row * 16, fg);
                } else {
                    window().pixels()->put_pixel(c + m_cursor_col * 8, r + m_cursor_row * 16, bg);
                }
            }
        }
    }

    window().draw();
}
#endif /* KERNEL_NO_GRRAPHICS */

void VgaBuffer::switch_to(UniquePtr<SaveState> state) {
    if (!state) {
        clear();
    } else {
        memcpy(buffer(), state->vector(), m_graphics_container.size_in_bytes());
    }
    set_cursor(m_cursor_row, m_cursor_col);
    refresh();
}

UniquePtr<VgaBuffer::SaveState> VgaBuffer::save_state() {
    auto ptr = UniquePtr<SaveState>(new SaveState(m_graphics_container.size()));
    for (size_t i = 0; i < m_graphics_container.size(); i++) {
        ptr->add(buffer()[i]);
    }
    return move(ptr);
}

void VgaBuffer::draw(int row, int col, char c) {
    draw(row, col, VGA_ENTRY(c, fg(), bg()));
}

#ifdef KERNEL_NO_GRAPHICS
void VgaBuffer::draw(int row, int col, uint16_t val) {
    buffer()[row * width() + col] = val;
}
#else
void VgaBuffer::draw(int row, int col, char ch, VgaColor fg, VgaColor bg) {
    Renderer renderer(*window().pixels());
    char text[2];
    text[0] = ch;
    text[1] = '\0';

    renderer.fill_rect(col * 8, row * 16, 8, 16, Color(bg));
    renderer.render_text(col * 8, row * 16, text, Color(fg), m_bold ? Font::bold_font() : Font::default_font());

    if (m_cursor_was_drawn && row == m_drawn_cursor_row && col == m_drawn_cursor_col) {
        for (int r = 0; r < 16; r++) {
            for (int c = 0; c < 8; c++) {
                m_cursor_location_save[r][c] = window().pixels()->get_pixel(c + m_drawn_cursor_col * 8, r + m_drawn_cursor_row * 16);
            }
        }
    }
}
#endif /* KERNEL_NO_GRAPHICS */

void VgaBuffer::clear_row_to_end(int row, int col) {
    for (int c = col; c < width(); c++) {
        draw(row, c, ' ');
    }
}

void VgaBuffer::clear_row(int row) {
    clear_row_to_end(row, 0);
}

VgaBuffer::Row VgaBuffer::scroll_up(const Row* replacement) {
#ifdef KERNEL_NO_GRAPHICS
    LIIM::Vector<uint16_t> first_row(buffer(), width());

    for (int r = 0; r < height() - 1; r++) {
        for (int c = 0; c < width(); c++) {
            draw(r, c, buffer()[(r + 1) * width() + c]);
        }
    }

    if (!replacement) {
        clear_row(height() - 1);
    } else {
        memcpy(buffer() + (height() - 1) * width(), replacement->vector(), m_graphics_container.row_size_in_bytes());
    }
    return first_row;
#else
    restore_cursor_pixels();

    Row first_row(window().pixels()->pixels(), width() * 16 * 8);

    size_t raw_offset = (height() - 1) * width() * 16 * 8;
    memmove(window().pixels()->pixels(), window().pixels()->pixels() + width() * 16 * 8, raw_offset * sizeof(uint32_t));

    if (!replacement) {
        clear_row(height() - 1);
    } else {
        memcpy(window().pixels()->pixels() + raw_offset, replacement->vector(), width() * 16 * 8 * sizeof(uint32_t));
    }
    return first_row;
#endif /* KERNEL_NO_GRAPHICS */
}

VgaBuffer::Row VgaBuffer::scroll_down(const Row* replacement) {
#ifdef KERNEL_NO_GRAPHICS
    Row last_row(buffer() + (height() - 1) * width(), width());

    for (int r = height() - 1; r > 0; r--) {
        for (int c = 0; c < width(); c++) {
            draw(r, c, buffer()[(r - 1) * width() + c]);
        }
    }
    if (!replacement) {
        clear_row(0);
    } else {
        memcpy(buffer(), replacement->vector(), m_graphics_container.row_size_in_bytes());
    }
    return last_row;
#else
    restore_cursor_pixels();

    size_t raw_offset = (height() - 1) * width() * 16 * 8;
    Row last_row(window().pixels()->pixels() + raw_offset, width() * 16 * 8);

    memmove(window().pixels()->pixels() + width() * 16 * 8, window().pixels()->pixels(), raw_offset * sizeof(uint32_t));

    if (!replacement) {
        clear_row(0);
    } else {
        memcpy(window().pixels()->pixels(), replacement->vector(), width() * 16 * 8 * sizeof(uint32_t));
    }
    return last_row;
#endif /* KERNEL_NO_GRAPHICS */
}

void VgaBuffer::clear() {
    for (int r = 0; r < height(); r++) {
        clear_row(r);
    }
}

void VgaBuffer::set_cursor(int row, int col) {
#ifdef KERNEL_NO_GRAPHICS
    cursor_pos pos = { static_cast<unsigned short>(row), static_cast<unsigned short>(col) };
    ioctl(m_graphics_container.fb(), SSCURSOR, &pos);
#endif /* KERNEL_NO_GRAPHICS */
    m_cursor_row = row;
    m_cursor_col = col;
}

void VgaBuffer::hide_cursor() {
    if (!m_is_cursor_enabled) {
        return;
    }

#ifdef KERNEL_NO_GRAPHICS
    ioctl(m_graphics_container.fb(), SDCURSOR);
#endif /* KERNEL_NO_GRAPHICS */

    m_is_cursor_enabled = false;
}

void VgaBuffer::show_cursor() {
#ifdef KERNEL_NO_GRAPHICS
    ioctl(m_graphics_container.fb(), SECURSOR);
#endif /* KERNEL_NO_GRAPHICS */
    m_is_cursor_enabled = true;
}
