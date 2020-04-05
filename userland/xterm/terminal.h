#pragma once

#include <liim/pointers.h>
#include <sys/types.h>

#include "tty.h"
#include "vga_buffer.h"

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

    void load(VgaBuffer::GraphicsContainer& container);

    int mfd() const { return m_mfd; }
    pid_t pid() const { return m_pid; }

    TTY& tty() { return *m_tty; }
    const TTY& tty() const { return *m_tty; }

    VgaBuffer& vga_buffer() { return *m_buffer; }
    const VgaBuffer& vga_buffer() const { return *m_buffer; }

    void save();
    void switch_to();
    void reset();

private:
    UniquePtr<VgaBuffer::SaveState> m_save_state;
    UniquePtr<VgaBuffer> m_buffer;
    UniquePtr<TTY> m_tty;
    int m_mfd { -1 };
    pid_t m_pid { -1 };
};