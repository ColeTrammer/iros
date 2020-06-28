#include <assert.h>
#include <fcntl.h>
#include <graphics/pixel_buffer.h>
#include <stdio.h>
#include <sys/mman.h>

#include "window.h"
#include "window_manager.h"

static wid_t get_next_id() {
    static wid_t next_wid = 1;
    return next_wid++;
}

Window::Window(String shm_path, const Rect& rect, String title, int client_id)
    : m_shm_path(move(shm_path)), m_content_rect(rect), m_id(get_next_id()), m_title(title), m_client_id(client_id) {
    m_rect.set_x(rect.x() - 1);
    m_rect.set_y(rect.y() - 22);
    m_rect.set_width(rect.width() + 2);
    m_rect.set_height(rect.height() + 23);
    map_buffers();
}

Window::~Window() {
    if (m_front_buffer && m_back_buffer && !m_shm_path.is_empty()) {
        munmap(m_back_buffer->pixels(), m_back_buffer->size_in_bytes());
        shm_unlink(m_shm_path.string());
        m_shm_path[m_shm_path.size() - 1]++;
        munmap(m_front_buffer->pixels(), m_front_buffer->size_in_bytes());
        shm_unlink(m_shm_path.string());
    }
}

void Window::map_buffers() {
    auto path = m_shm_path;
    {
        int fd = shm_open(path.string(), O_RDWR | O_CREAT, 0666);
        assert(fd != -1);

        size_t len = m_content_rect.width() * m_content_rect.height() * sizeof(uint32_t);
        ftruncate(fd, len);

        void* memory = mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        assert(memory != MAP_FAILED);

        m_front_buffer = PixelBuffer::wrap(reinterpret_cast<uint32_t*>(memory), m_content_rect.width(), m_content_rect.height());
        m_front_buffer->clear();

        close(fd);
    }
    {
        path[path.size() - 1]++;
        int fd = shm_open(path.string(), O_RDWR | O_CREAT, 0666);
        assert(fd != -1);

        size_t len = m_content_rect.width() * m_content_rect.height() * sizeof(uint32_t);
        ftruncate(fd, len);

        void* memory = mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        assert(memory != MAP_FAILED);

        m_back_buffer = PixelBuffer::wrap(reinterpret_cast<uint32_t*>(memory), m_content_rect.width(), m_content_rect.height());
        m_back_buffer->clear();

        close(fd);
    }
}

void Window::relative_resize(int delta) {
    if (delta == 0) {
        return;
    }

    if (m_front_buffer) {
        munmap(m_front_buffer->pixels(), m_front_buffer->size_in_bytes());
    }
    if (m_back_buffer) {
        munmap(m_back_buffer->pixels(), m_back_buffer->size_in_bytes());
    }

    if (delta < 0) {
        WindowManager::the().invalidate_rect(m_rect);
    }

    m_content_rect.set_width(m_content_rect.width() + delta);
    m_rect.set_width(m_rect.width() + delta);

    if (delta > 0) {
        WindowManager::the().invalidate_rect(m_rect);
    }

    map_buffers();
}

void Window::set_x(int x) {
    m_rect.set_x(x);
    m_content_rect.set_x(x + 1);
}

void Window::set_y(int y) {
    m_rect.set_y(y);
    m_content_rect.set_y(y + 22);
}

void Window::swap() {
    auto temp = m_back_buffer;
    m_back_buffer = m_front_buffer;
    m_front_buffer = temp;

    WindowManager::the().invalidate_rect(m_content_rect);
}
