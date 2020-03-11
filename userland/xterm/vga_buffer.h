#pragma once

#include <liim/vector.h>
#include <stddef.h>
#include <stdint.h>
#ifndef KERNEL_NO_GRAPHICS
#include <graphics/color.h>
#include <graphics/renderer.h>
#include <liim/pointers.h>
#include <window_server/connection.h>
#include <window_server/window.h>

class Renderer;
#endif /* KERNEL_NO_GRAPHICS */

#include <kernel/hal/x86_64/drivers/vga.h>

class VgaBuffer {
public:
#ifdef KERNEL_NO_GRAPHICS
    using Row = Vector<uint16_t>;
    using VgaColor = enum vga_color;
#else
    using Row = Vector<uint32_t>;
    using VgaColor = Color;
#endif /* KERNEL_NO_GRAPHICS */

    VgaBuffer(const char* path);
    ~VgaBuffer();

    size_t size() const { return m_width * m_height; }
#ifdef KERNEL_NO_GRAPHICS
    size_t row_size_in_bytes() const { return m_width * sizeof(uint16_t); }
    size_t size_in_bytes() const { return size() * sizeof(uint16_t); }
#endif /* KERNEL_NO_GRAPHICS */

    int width() const { return m_width; }
    int height() const { return m_height; }

    VgaColor bg() const { return m_bg; }
    void set_bg(VgaColor bg) { m_bg = bg; }

    VgaColor fg() const { return m_fg; }
    void set_fg(VgaColor fg) { m_fg = fg; }

    void swap_colors() { LIIM::swap(m_fg, m_bg); }

    void reset_colors() {
        reset_fg();
        reset_bg();
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

private:
    bool m_is_cursor_enabled { true };
    int m_width { 80 };
    int m_height { 25 };
    VgaColor m_bg { VGA_COLOR_BLACK };
    VgaColor m_fg { VGA_COLOR_LIGHT_GREY };
#ifdef KERNEL_NO_GRAPHICS
    void draw(int row, int col, uint16_t value);

    uint16_t* m_buffer;
    const int m_fb;
#else
    void draw(int row, int col, char c, Color fg, Color bg);

    WindowServer::Connection m_connection;
    SharedPtr<WindowServer::Window> m_window { nullptr };
#endif /* KERNEL_NO_GRAPHICS */
};