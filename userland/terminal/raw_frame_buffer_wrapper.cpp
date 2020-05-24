#include <assert.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "raw_frame_buffer_wrapper.h"

RawFrameBufferWrapper::RawFrameBufferWrapper() : m_fb(open("/dev/fb0", O_RDWR | O_CLOEXEC)) {
    assert(m_fb != -1);
    assert(ioctl(m_fb, SGWIDTH, &m_width) == 0);
    assert(ioctl(m_fb, SGHEIGHT, &m_height) == 0);

    m_buffer = static_cast<uint16_t*>(mmap(nullptr, size_in_bytes(), PROT_READ | PROT_WRITE, MAP_SHARED, m_fb, 0));
    assert(m_buffer != MAP_FAILED);
}

RawFrameBufferWrapper::~RawFrameBufferWrapper() {
    munmap(m_buffer, size_in_bytes());
    close(m_fb);
}
