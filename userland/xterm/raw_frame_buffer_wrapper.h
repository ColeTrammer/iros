#pragma once

#include <stdint.h>

class RawFrameBufferWrapper {
public:
    RawFrameBufferWrapper();
    ~RawFrameBufferWrapper();

    uint16_t* buffer() { return m_buffer; }
    int fb() const { return m_fb; }

    size_t size() const { return m_width * m_height; }
    size_t row_size_in_bytes() const { return m_width * sizeof(uint16_t); }
    size_t size_in_bytes() const { return size() * sizeof(uint16_t); }

    int width() const { return m_width; }
    int height() const { return m_height; }

private:
    uint16_t* m_buffer;
    int m_fb;
    int m_width;
    int m_height;
};