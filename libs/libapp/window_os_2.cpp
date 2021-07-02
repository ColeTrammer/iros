#include <app/application_os_2.h>
#include <app/window_os_2.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

namespace App {
OSWindow::OSWindow(Window& window, int x, int y, int width, int height, String name, bool has_alpha, WindowServer::WindowType type,
                   wid_t parent_id)
    : m_window(window) {
    auto response = OSApplication::the()
                        .ws()
                        .server()
                        .send_then_wait<WindowServer::Client::CreateWindowRequest, WindowServer::Server::CreateWindowResponse>({
                            .x = x,
                            .y = y,
                            .width = width,
                            .height = height,
                            .name = move(name),
                            .type = type,
                            .parent_id = parent_id,
                            .has_alpha = has_alpha,
                        });
    assert(response.has_value());

    m_shm_path = response.value().path;
    m_window.set_id(response.value().wid);

    do_resize(response->width, response->height);
}

OSWindow::~OSWindow() {
    if (m_raw_pixels != MAP_FAILED) {
        munmap(m_raw_pixels, m_raw_pixels_size);
        m_raw_pixels = MAP_FAILED;
    }

    if (!m_window.removed()) {
        OSApplication::the().ws().server().send<WindowServer::Client::RemoveWindowRequest>({ .wid = m_window.wid() });
    }
}

void OSWindow::do_set_visibility(int x, int y, bool visible) {
    OSApplication::the()
        .ws()
        .server()
        .send_then_wait<WindowServer::Client::ChangeWindowVisibilityRequest, WindowServer::Server::ChangeWindowVisibilityResponse>({
            .wid = m_window.wid(),
            .x = x,
            .y = y,
            .visible = visible,
        });
}

void OSWindow::flush_pixels() {
    OSApplication::the().ws().server().send<WindowServer::Client::SwapBufferRequest>({ .wid = m_window.wid() });
    LIIM::swap(m_front_buffer, m_back_buffer);
    memcpy(m_back_buffer->pixels(), m_front_buffer->pixels(), m_front_buffer->size_in_bytes());
}

void OSWindow::did_resize() {
    auto response =
        OSApplication::the()
            .ws()
            .server()
            .send_then_wait<WindowServer::Client::WindowReadyToResizeMessage, WindowServer::Server::WindowReadyToResizeResponse>(
                { .wid = m_window.wid() });
    assert(response.has_value());
    auto& data = response.value();
    assert(data.wid == m_window.wid());

    if (m_window.rect().width() == data.new_width && m_window.rect().height() == data.new_height) {
        return;
    }

    do_resize(data.new_width, data.new_height);
}

void OSWindow::do_resize(int new_width, int new_height) {
    if (m_raw_pixels != MAP_FAILED) {
        munmap(m_raw_pixels, m_raw_pixels_size);
        m_raw_pixels = MAP_FAILED;
    }

    int shm = shm_open(m_shm_path.string(), O_RDWR, 0);
    assert(shm != -1);

    m_raw_pixels_size = 2 * new_width * new_height * sizeof(uint32_t);
    m_raw_pixels = mmap(nullptr, m_raw_pixels_size, PROT_WRITE | PROT_READ, MAP_SHARED, shm, 0);
    assert(m_raw_pixels != MAP_FAILED);
    close(shm);

    m_front_buffer = Bitmap::wrap(reinterpret_cast<uint32_t*>(m_raw_pixels), new_width, new_height, m_window.has_alpha());
    m_back_buffer = Bitmap::wrap(reinterpret_cast<uint32_t*>(m_raw_pixels) + m_raw_pixels_size / 2 / sizeof(uint32_t), new_width,
                                 new_height, m_window.has_alpha());

    m_window.set_rect({ 0, 0, new_width, new_height });
}
}
