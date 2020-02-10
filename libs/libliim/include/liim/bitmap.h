#pragma once

#include <liim/pointers.h>
#include <limits.h>
#include <string.h>

namespace LIIM {

template<typename T>
class Bitmap {
public:
    Bitmap(int num_bits) { m_bits = new T[(num_bits + sizeof(T) * CHAR_BIT - 1) / (sizeof(T) * CHAR_BIT)]; }

    template<typename U>
    static SharedPtr<Bitmap<U>> wrap(U* bits, int num_bits) {
        auto bitmap = make_shared<Bitmap<U>>();
        bitmap->m_should_deallocate = false;
        bitmap->m_bits = bits;
        return bitmap;
    }

    ~Bitmap() {
        if (m_should_deallocate) {
            delete[] m_bits;
        }
    }

    bool get(int bit_index) const {
        int long_index = bit_index / (sizeof(T) * CHAR_BIT);
        bit_index %= sizeof(T) * CHAR_BIT;
        return (m_bits[long_index] & (1UL << bit_index)) ? true : false;
    }

    void set(int bit_index) {
        int long_index = bit_index / (sizeof(T) * CHAR_BIT);
        bit_index %= sizeof(T) * CHAR_BIT;
        m_bits[long_index] |= (1UL << bit_index);
    }

    void unset(int bit_index) {
        int long_index = bit_index / (sizeof(T) * CHAR_BIT);
        bit_index %= sizeof(T) * CHAR_BIT;
        m_bits[long_index] &= ~(1UL << bit_index);
    }

    void flip(int bit_index) {
        int long_index = bit_index / (sizeof(T) * CHAR_BIT);
        bit_index %= sizeof(T) * CHAR_BIT;
        m_bits[long_index] ^= (1UL << bit_index);
    }

    T* bitmap() { return m_bits; }
    const T* bitmap() const { return m_bits; }

    Bitmap() {}

private:
    bool m_should_deallocate { true };
    T* m_bits { nullptr };
};

}

using LIIM::Bitmap;