#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <kernel/hal/x86_64/drivers/vga.h>

#include "vga_buffer.h"

VgaBuffer::VgaBuffer(const char* path)
    : m_fb(open(path, O_RDWR))
{
    assert(m_fb != -1);

    assert(ioctl(m_fb, SGWIDTH, &m_width) == 0);
    assert(ioctl(m_fb, SGHEIGHT, &m_height) == 0);

    m_buffer = static_cast<uint16_t*>(mmap(nullptr, size_in_bytes(), PROT_READ | PROT_WRITE, MAP_SHARED, m_fb, 0));
    assert(m_buffer != MAP_FAILED);

    clear();
    set_cursor(0, 0);
}

VgaBuffer::~VgaBuffer()
{
    munmap(m_buffer, size_in_bytes());
    close(m_fb);
}

void VgaBuffer::draw(int row, int col, char c)
{
    draw(row, col, (uint16_t) VGA_ENTRY(c, fg(), bg()));
}

void VgaBuffer::draw(int row, int col, uint16_t val)
{
    m_buffer[row * m_width + col] = val;
}

void VgaBuffer::hide_cursor()
{
    if (!m_is_cursor_enabled) {
        return;
    }

    ioctl(m_fb, SDCURSOR);
    m_is_cursor_enabled = false;
}

void VgaBuffer::show_cursor()
{
    if (m_is_cursor_enabled) {
        return;
    }

    ioctl(m_fb, SECURSOR);
    m_is_cursor_enabled = true;
}

void VgaBuffer::clear_row_to_end(int row, int col)
{
    for (int c = col; c < m_width; c++) {
        draw(row, c, ' ');
    }
}

void VgaBuffer::clear_row(int row)
{
    clear_row_to_end(row, 0);
}

uint16_t* VgaBuffer::scroll_up(const uint16_t* replacement)
{
    uint16_t* first_row = new uint16_t[m_width];
    memcpy(first_row, m_buffer, row_size_in_bytes());

    for (int r = 0; r < m_height - 1; r++) {
        for (int c = 0; c < m_width; c++) {
            draw(r, c, m_buffer[(r + 1) * m_width + c]);
        }
    }

    if (!replacement) {
        clear_row(m_height - 1);
    } else {
        memcpy(m_buffer + (m_height - 1) * m_width, replacement, row_size_in_bytes());
    }
    return first_row;
}

uint16_t* VgaBuffer::scroll_down(const uint16_t* replacement)
{
    uint16_t* last_row = new uint16_t[m_width];
    memcpy(last_row, m_buffer + (m_height - 1) * m_width, row_size_in_bytes());

    for (int r = m_height - 1; r > 0; r--) {
        for (int c = 0; c < m_width; c++) {
            draw(r, c, m_buffer[(r - 1) * m_width + c]);
        }
    }
    memcpy(m_buffer, replacement, row_size_in_bytes());
    return last_row;
}


void VgaBuffer::clear()
{
    for (int r = 0; r < m_height; r++) {
        clear_row(r);
    }
}

void VgaBuffer::set_cursor(int row, int col)
{
    cursor_pos pos = { static_cast<unsigned short>(row), static_cast<unsigned short>(col) };
    ioctl(m_fb, SSCURSOR, &pos);
}