#pragma once

#include <liim/pointers.h>
#include <sys/types.h>

class TTY;
class VgaBuffer;

class Terminal {
public:
    Terminal() {}
    Terminal(const Terminal&) = delete;

    Terminal(Terminal&& other) : m_buffer(move(other.m_buffer)), m_tty(move(other.m_tty)), m_mfd(other.m_mfd), m_pid(other.m_pid) {
        other.m_mfd = -1;
        other.m_pid = -1;
    }

    ~Terminal();

    bool is_loaded() const { return m_buffer; }

    void load();

    int mfd() const { return m_mfd; }

    TTY& tty() { return *m_tty; }
    const TTY& tty() const { return *m_tty; }

private:
    UniquePtr<VgaBuffer> m_buffer;
    UniquePtr<TTY> m_tty;
    int m_mfd { -1 };
    pid_t m_pid { -1 };
};