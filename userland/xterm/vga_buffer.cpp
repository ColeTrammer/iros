#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <kernel/hal/x86_64/drivers/vga.h>

#include "vga_buffer.h"

VgaBuffer::VgaBuffer(const char* path)
{
    m_fb = open(path, O_RDWR);
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
    m_buffer[VGA_INDEX(row, col)] = VGA_ENTRY(c, fg(), bg());
}

void VgaBuffer::hide_cursor()
{
    ioctl(m_fb, SDCURSOR);
}

void VgaBuffer::show_cursor()
{
    ioctl(m_fb, SECURSOR);
}

void VgaBuffer::clear_row(int row)
{
    for (int c = 0; c < m_width; c++) {
        draw(row, c, ' ');
    }
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