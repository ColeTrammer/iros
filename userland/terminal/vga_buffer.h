#pragma once

#include <graphics/color.h>
#include <stdint.h>

class VgaBuffer {
public:
    VgaBuffer();
    ~VgaBuffer();

    uint16_t* buffer() { return m_buffer; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    int fb() const { return m_fb; }
    size_t size_in_bytes() const { return m_width * m_height * sizeof(uint16_t); }

    void show_cursor(int row, int col);
    void hide_cursor();
    bool cursor_is_hidden() const { return m_cursor_is_hidden; }

    void draw(int r, int c, vga_color bg, vga_color fg, char ch);

private:
    uint16_t* m_buffer { nullptr };
    int m_width { 0 };
    int m_height { 0 };
    int m_cursor_row { -1 };
    int m_cursor_col { -1 };
    int m_fb { -1 };
    bool m_cursor_is_hidden { false };
};
