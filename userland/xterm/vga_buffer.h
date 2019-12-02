#pragma once

#include <liim/vector.h>
#include <stddef.h>
#include <stdint.h>

#include <kernel/hal/x86_64/drivers/vga.h>

class VgaBuffer {
public:
    VgaBuffer(const char *path);
    ~VgaBuffer();

    size_t size() const {
        return m_width * m_height;
    }
    size_t row_size_in_bytes() const {
        return m_width * sizeof(uint16_t);
    }
    size_t size_in_bytes() const {
        return size() * sizeof(uint16_t);
    }

    int width() const {
        return m_width;
    }
    int height() const {
        return m_height;
    }

    enum vga_color bg() const {
        return m_bg;
    }
    void set_bg(enum vga_color bg) {
        m_bg = bg;
    }

    enum vga_color fg() const {
        return m_fg;
    }
    void set_fg(enum vga_color fg) {
        m_fg = fg;
    }

    void swap_colors() {
        enum vga_color t = m_bg;
        m_bg = m_fg;
        m_fg = t;
    }

    void reset_colors() {
        reset_fg();
        reset_bg();
    }

    void reset_fg() {
        m_fg = VGA_COLOR_LIGHT_GREY;
    }
    void reset_bg() {
        m_bg = VGA_COLOR_BLACK;
    }

    void clear_row_to_end(int row, int col);
    void clear_row(int row);
    void clear();

    void draw(int row, int col, char c);
    void draw(int row, int col, uint16_t val);

    LIIM::Vector<uint16_t> scroll_up(const LIIM::Vector<uint16_t> *first_row = nullptr);
    LIIM::Vector<uint16_t> scroll_down(const LIIM::Vector<uint16_t> *last_row = nullptr);

    void show_cursor();
    void hide_cursor();
    void set_cursor(int row, int col);

private:
    bool m_is_cursor_enabled { true };
    int m_width;
    int m_height;
    const int m_fb;
    enum vga_color m_bg { VGA_COLOR_BLACK };
    enum vga_color m_fg { VGA_COLOR_LIGHT_GREY };
    uint16_t *m_buffer;
};