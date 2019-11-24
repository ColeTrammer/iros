#include <assert.h>
#include <fcntl.h>
#include <graphics/pixel_buffer.h>
#include <stdio.h>
#include <sys/mman.h>

#include "window.h"

Window::Window(const String& shm_path, const Rect& rect) 
    : m_shm_path(shm_path)
    , m_rect(rect)
{
    int fd = shm_open(shm_path.string(), O_RDWR | O_CREAT | O_EXCL, 0666);

    size_t len = rect.width() * rect.height() * sizeof(uint16_t); 
    ftruncate(fd, len);

    void* memory = mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    m_buffer = PixelBuffer::wrap(reinterpret_cast<uint32_t*>(memory), m_rect.width(), m_rect.height());

    close(fd);
}

Window::Window(const Window &other) 
    : m_rect(other.rect())
    , m_buffer(other.buffer())
{
}

Window::~Window()
{
    if (m_buffer && m_shm_path) {
        munmap(m_buffer->pixels(), m_buffer->size_in_bytes());
        shm_unlink(m_shm_path.string());
    }
}