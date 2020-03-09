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

#ifdef KERNEL_NO_GRAPHICS
VgaBuffer::VgaBuffer(const char* path) : m_fb(open(path, O_RDWR)) {
    assert(m_fb != -1);

    assert(ioctl(m_fb, SGWIDTH, &m_width) == 0);
    assert(ioctl(m_fb, SGHEIGHT, &m_height) == 0);

    m_buffer = static_cast<uint16_t*>(mmap(nullptr, size_in_bytes(), PROT_READ | PROT_WRITE, MAP_SHARED, m_fb, 0));
    assert(m_buffer != MAP_FAILED);

    clear();
    set_cursor(0, 0);
}

VgaBuffer::~VgaBuffer() {
    munmap(m_buffer, size_in_bytes());
    close(m_fb);
}

void VgaBuffer::refresh() {}

#else

static Color convert(vga_color color) {
    switch (color) {
        case VGA_COLOR_BLACK:
            return Color(0, 0, 0);
        case VGA_COLOR_RED:
            return Color(170, 0, 0);
        case VGA_COLOR_GREEN:
            return Color(0, 170, 0);
        case VGA_COLOR_BROWN:
            return Color(170, 85, 0);
        case VGA_COLOR_BLUE:
            return Color(0, 0, 170);
        case VGA_COLOR_MAGENTA:
            return Color(170, 0, 170);
        case VGA_COLOR_CYAN:
            return Color(0, 170, 170);
        case VGA_COLOR_LIGHT_GREY:
            return Color(170, 170, 170);
        case VGA_COLOR_DARK_GREY:
            return Color(85, 85, 85);
        case VGA_COLOR_LIGHT_RED:
            return Color(255, 85, 85);
        case VGA_COLOR_LIGHT_GREEN:
            return Color(85, 255, 85);
        case VGA_COLOR_YELLOW:
            return Color(255, 255, 85);
        case VGA_COLOR_LIGHT_BLUE:
            return Color(85, 85, 255);
        case VGA_COLOR_LIGHT_MAGENTA:
            return Color(255, 85, 255);
        case VGA_COLOR_LIGHT_CYAN:
            return Color(85, 255, 255);
        case VGA_COLOR_WHITE:
            return Color(255, 255, 255);
        default:
            return Color(255, 255, 255);
    }
}

void VgaBuffer::update_entry(int r, int c) {
    Renderer renderer(*m_window->pixels());
    uint16_t vga_pixel = m_buffer[r * m_width + c];
    vga_color vback = (vga_color)((vga_pixel & 0xF000) >> 12);
    vga_color vfront = (vga_color)((vga_pixel & 0x0F00) >> 8);
    char ch = vga_pixel & 0xFF;
    Color b = convert(vback);
    Color f = convert(vfront);
    char text[2];
    text[0] = ch;
    text[1] = '\0';

    renderer.set_color(b);
    renderer.fill_rect(c * 8, r * 16, 8, 16);

    renderer.set_color(f);
    renderer.render_text(c * 8, r * 16, text);
}

VgaBuffer::VgaBuffer(const char*) {
    m_window = m_connection.create_window(200, 200, m_width * 8, m_height * 16);
    m_buffer = new uint16_t[m_width * m_height];
    for (int r = 0; r < m_height; r++) {
        for (int c = 0; c < m_width; c++) {
            m_buffer[r * m_width + c] = VGA_ENTRY(' ', VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY);
        }
    }
}

VgaBuffer::~VgaBuffer() {
    delete[] m_buffer;
}

void VgaBuffer::refresh() {
    m_window->draw();
}

#endif /* KERNEL_NO_GRRAPHICS */

void VgaBuffer::draw(int row, int col, char c) {
    draw(row, col, (uint16_t) VGA_ENTRY(c, fg(), bg()));
}

void VgaBuffer::draw(int row, int col, uint16_t val) {
    m_buffer[row * m_width + col] = val;
#ifndef KERNEL_NO_GRAPHICS
    update_entry(row, col);
#endif /* KERNEL_NO_GRAPHICS */
}

void VgaBuffer::clear_row_to_end(int row, int col) {
    for (int c = col; c < m_width; c++) {
        draw(row, c, ' ');
    }
}

void VgaBuffer::clear_row(int row) {
    clear_row_to_end(row, 0);
}

LIIM::Vector<uint16_t> VgaBuffer::scroll_up(const LIIM::Vector<uint16_t>* replacement) {
    LIIM::Vector<uint16_t> first_row(m_buffer, m_width);

    for (int r = 0; r < m_height - 1; r++) {
        for (int c = 0; c < m_width; c++) {
            draw(r, c, m_buffer[(r + 1) * m_width + c]);
        }
    }

    if (!replacement) {
        clear_row(m_height - 1);
    } else {
        memcpy(m_buffer + (m_height - 1) * m_width, replacement->vector(), row_size_in_bytes());
    }
    return first_row;
}

LIIM::Vector<uint16_t> VgaBuffer::scroll_down(const LIIM::Vector<uint16_t>* replacement) {
    LIIM::Vector<uint16_t> last_row(m_buffer + (m_height - 1) * m_width, m_width);

    for (int r = m_height - 1; r > 0; r--) {
        for (int c = 0; c < m_width; c++) {
            draw(r, c, m_buffer[(r - 1) * m_width + c]);
        }
    }
    if (!replacement) {
        clear_row(0);
    } else {
        memcpy(m_buffer, replacement->vector(), row_size_in_bytes());
    }
    return last_row;
}

void VgaBuffer::clear() {
    for (int r = 0; r < m_height; r++) {
        clear_row(r);
    }
}

void VgaBuffer::set_cursor(int row [[maybe_unused]], int col [[maybe_unused]]) {
#ifdef KERNEL_NO_GRAPHICS
    cursor_pos pos = { static_cast<unsigned short>(row), static_cast<unsigned short>(col) };
    ioctl(m_fb, SSCURSOR, &pos);
#endif /* KERNEL_NO_GRAPHICS */
}

void VgaBuffer::hide_cursor() {
    if (!m_is_cursor_enabled) {
        return;
    }

#ifdef KERNEL_NO_GRAPHICS
    ioctl(m_fb, SDCURSOR);
#endif /* KERNEL_NO_GRAPHICS */

    m_is_cursor_enabled = false;
}

void VgaBuffer::show_cursor() {
#ifdef KERNEL_NO_GRAPHICS
    ioctl(m_fb, SECURSOR);
#endif /* KERNEL_NO_GRAPHICS */
    m_is_cursor_enabled = true;
}