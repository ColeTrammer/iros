#pragma once

#include <liim/string.h>
#include <liim/traits.h>
#include <liim/vector.h>
#include <stddef.h>
#include <string.h>

namespace IPC {

template<typename T>
struct Serializer {
    constexpr static uint32_t serialization_size(const T& val) {
        static_assert(LIIM::Traits<T>::is_simple());
        return sizeof(val);
    }
};

template<>
struct Serializer<String> {
    static uint32_t serialization_size(const String& val) { return sizeof(val.size()) + val.size(); }
};

template<typename T>
struct Serializer<Vector<T>> {
    static uint32_t serialization_size(const Vector<T>& val) {
        uint32_t ret = sizeof(val.size());
        for (auto& v : val) {
            ret += Serializer<T>::serialization_size(v);
        }
        return ret;
    }
};

class Stream {
public:
    Stream(char* buffer, size_t buffer_max) : m_buffer(buffer), m_buffer_max(buffer_max) {}

    const char* buffer() const { return m_buffer; }
    size_t buffer_size() const { return m_buffer_index; }
    size_t buffer_max() const { return m_buffer_max; }
    bool error() const { return m_error; }
    void set_error() { m_error = true; }
    void rewind() {
        m_buffer_index = 0;
        m_error = false;
    }

    template<typename TrivialType>
    Stream& operator<<(const TrivialType& val) {
        static_assert(LIIM::Traits<TrivialType>::is_simple());
        if (Serializer<TrivialType>::serialization_size(val) + m_buffer_index > m_buffer_max) {
            set_error();
            return *this;
        }

        memcpy(m_buffer + m_buffer_index, &val, sizeof(val));
        m_buffer_index += sizeof(val);
        return *this;
    }

    template<typename TrivialType>
    Stream& operator>>(TrivialType& val) {
        static_assert(LIIM::Traits<TrivialType>::is_simple());
        if (Serializer<TrivialType>::serialization_size(val) + m_buffer_index > m_buffer_max) {
            set_error();
            val = {};
            return *this;
        }

        memcpy(&val, m_buffer + m_buffer_index, sizeof(val));
        m_buffer_index += sizeof(val);
        return *this;
    }

    Stream& operator<<(const String& string) {
        *this << string.size();
        for (auto c : string) {
            *this << c;
        }
        return *this;
    }

    Stream& operator>>(String& string) {
        string = {};

        size_t size;
        *this >> size;
        for (size_t i = 0; !error() && i < size; i++) {
            char c;
            *this >> c;
            string.insert(c, string.size());
        }
        return *this;
    }

    template<typename T>
    Stream& operator<<(const Vector<T>& val) {
        *this << val.size();
        for (auto& v : val) {
            *this << v;
        }
        return *this;
    }

    template<typename T>
    Stream& operator>>(Vector<T>& val) {
        val = {};

        int size;
        *this >> size;
        for (int i = 0; !error() && i < size; i++) {
            T v;
            *this >> v;
            val.add(move(v));
        }
        return *this;
    }

private:
    char* m_buffer { nullptr };
    size_t m_buffer_max { 0 };
    size_t m_buffer_index { 0 };
    bool m_error { false };
};
}
