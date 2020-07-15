#pragma once

#include <liim/string.h>
#include <sys/types.h>
#include <kernel/hal/input.h>

class PsuedoTerminal {
public:
    PsuedoTerminal();
    ~PsuedoTerminal();

    int master_fd() const { return m_master_fd; }
    pid_t child_pid() const { return m_child_pid; }

    void send_clipboard_contents(const String& contents);
    void handle_key_event(key key, int flags, char ascii);
    void set_size(int rows, int cols);

    void write(const String& message);

private:
    int m_master_fd { -1 };
    int m_rows { 25 };
    int m_cols { 80 };
    pid_t m_child_pid { -1 };
};
