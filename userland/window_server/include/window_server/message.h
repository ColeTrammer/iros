#pragma once

#include <liim/string.h>

struct WindowServerMessage {
    enum class Type {
        Invalid,
        Begin
    };

    Type type() const { return m_type; }

    const char* message() const { return m_message; }
    char* message() { return m_message; }

    Type m_type { Type::Invalid };
    char m_message[256] { 0 };

    WindowServerMessage()
    {   
    }

    WindowServerMessage(Type type, const String& message)
        : m_type(type)
    {
        strcpy(m_message, message.string());
    }

    ~WindowServerMessage()
    {
    }
};