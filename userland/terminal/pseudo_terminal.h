#pragma once

#include <sys/types.h>
#include <kernel/hal/input.h>

class PsuedoTerminal {
public:
    PsuedoTerminal();
    ~PsuedoTerminal();

    int master_fd() const { return m_master_fd; }
    pid_t child_pid() const { return m_child_pid; }

    void handle_key_event(key key, int flags, char ascii);

private:
    int m_master_fd { -1 };
    pid_t m_child_pid { -1 };
};
