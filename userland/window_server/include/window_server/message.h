#pragma once

#include <liim/string.h>
#include <stdint.h>

struct WindowServerMessage {
    enum class Type {
        Invalid,
        Begin,
        CreateWindow,
        CreatedWindow
    };

    struct CreateWindowData {
        int x;
        int y;
        int width;
        int height;
    };

    struct CreatedWindowData {
        char shared_buffer_path[0];
    };

    Type type() const { return m_type; }
    void set_type(Type type) { m_type = type; }

    int data_len() const { return m_data_len; }
    void set_data_len(int data_len) { m_data_len = data_len; }

    const uint8_t* data() const { return m_data; }
    uint8_t* data() { return m_data; }

    WindowServerMessage()
    {
    }

    ~WindowServerMessage()
    {
    }

    Type m_type { Type::Invalid };
    int m_data_len;
    uint8_t m_data[0];
};