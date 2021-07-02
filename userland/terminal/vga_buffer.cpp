#ifdef __os_2__
#include <assert.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "vga_buffer.h"

VgaBuffer::VgaBuffer() : m_fb(open("/dev/fb0", O_RDWR | O_CLOEXEC)) {
    assert(m_fb != -1);
    assert(ioctl(m_fb, SGWIDTH, &m_width) == 0);
    assert(ioctl(m_fb, SGHEIGHT, &m_height) == 0);

    m_buffer = static_cast<uint16_t*>(mmap(nullptr, size_in_bytes(), PROT_READ | PROT_WRITE, MAP_SHARED, m_fb, 0));
    assert(m_buffer != MAP_FAILED);
}

VgaBuffer::~VgaBuffer() {
    munmap(m_buffer, size_in_bytes());
    close(m_fb);
}

void VgaBuffer::draw(int r, int c, vga_color bg, vga_color fg, char ch) {
    m_buffer[r * m_width + c] = VGA_ENTRY(ch, fg, bg);
}

void VgaBuffer::show_cursor(int row, int col) {
    if (cursor_is_hidden()) {
        m_cursor_is_hidden = false;
        ioctl(m_fb, SECURSOR);
    }

    if (m_cursor_row == row && m_cursor_col == col) {
        return;
    }

    cursor_pos pos;
    pos.cp_row = row;
    pos.cp_col = col;
    ioctl(m_fb, SSCURSOR, &pos);
}

void VgaBuffer::hide_cursor() {
    if (cursor_is_hidden()) {
        return;
    }

    m_cursor_is_hidden = true;
    ioctl(m_fb, SDCURSOR);
}
#endif /* __os_2__ */
