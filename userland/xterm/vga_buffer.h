#pragma once

#include <liim/pointers.h>
#include <liim/vector.h>
#include <stddef.h>
#include <stdint.h>
#ifdef KERNEL_NO_GRAPHICS
#include "raw_frame_buffer_wrapper.h"
#else
#include <graphics/color.h>
#include "window_wrapper.h"
#endif /* KERNEL_NO_GRAPHICS */

#include <kernel/hal/x86_64/drivers/vga.h>
#ifndef KERNEL_NO_GRAPHICS
#undef VGA_ENTRY
#define VGA_ENTRY(c, f, b) c, f, b
#endif /* KERNEL_NO_GRAPHICS */

class VgaBuffer {
public:
#ifdef KERNEL_NO_GRAPHICS
    using Row = Vector<uint16_t>;
    using VgaColor = enum vga_color;
    using GraphicsContainer = RawFrameBufferWrapper;
    using SaveState = Vector<uint16_t>;
#else
    using Row = Vector<uint32_t>;
    using VgaColor = Color;
    using GraphicsContainer = WindowWrapper;
    using SaveState = Vector<uint32_t>;
#endif /* KERNEL_NO_GRAPHICS */

    VgaBuffer(GraphicsContainer& container);
    ~VgaBuffer();

    VgaColor bg() const { return m_bg; }
    void set_bg(VgaColor bg) { m_bg = bg; }

    VgaColor fg() const { return m_fg; }
    void set_fg(VgaColor fg) { m_fg = fg; }

    void swap_colors() { LIIM::swap(m_fg, m_bg); }

    void reset_colors() {
        reset_fg();
        reset_bg();
        set_bold(false);
    }

    void reset_fg() { m_fg = VGA_COLOR_LIGHT_GREY; }
    void reset_bg() { m_bg = VGA_COLOR_BLACK; }

    void clear_row_to_end(int row, int col);
    void clear_row(int row);
    void clear();

    void draw(int row, int col, char c);

    Row scroll_up(const Row* first_row = nullptr);
    Row scroll_down(const Row* last_row = nullptr);

    void show_cursor();
    void hide_cursor();
    void set_cursor(int row, int col);

    void refresh();

    void set_bold(bool value) { m_bold = value; }

    int width() const { return m_graphics_container.width(); }
    int height() const { return m_graphics_container.height(); }

    void switch_to(UniquePtr<SaveState> state);
    UniquePtr<SaveState> save_state();

private:
#ifdef KERNEL_NO_GRAPHICS
    uint16_t* buffer() { return m_graphics_container.buffer(); }
    void draw(int row, int col, uint16_t value);
#else
    WindowServer::Window& window() { return m_graphics_container.window(); }
    void draw(int row, int col, char c, Color fg, Color bg);
    uint32_t* buffer() { return window().pixels()->pixels(); }
#endif /* KERNEL_NO_GRAPHICS */

    bool m_is_cursor_enabled { true };
    bool m_bold { false };
    VgaColor m_bg { VGA_COLOR_BLACK };
    VgaColor m_fg { VGA_COLOR_LIGHT_GREY };
    int m_cursor_row { 0 };
    int m_cursor_col { 0 };
    GraphicsContainer& m_graphics_container;
};