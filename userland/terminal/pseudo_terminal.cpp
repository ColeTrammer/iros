#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "pseudo_terminal.h"

PsuedoTerminal::PsuedoTerminal() {
    m_master_fd = posix_openpt(O_RDWR);
    if (m_master_fd == -1) {
        perror("terminal: posix_openpt");
        exit(1);
    }

    m_child_pid = fork();
    if (m_child_pid < 0) {
        perror("terminal: fork");
        exit(1);
    }

    if (m_child_pid == 0) {
        int slave_fd = open(ptsname(m_master_fd), O_RDWR);
        if (slave_fd == -1) {
            perror("terminal (fork): open");
            _exit(1);
        }

        if (setsid() < 0) {
            perror("terminal (fork): setsid");
            _exit(1);
        }

        if (tcsetpgrp(m_master_fd, getpid())) {
            perror("terminal (fork): tcsetpgrp");
            _exit(1);
        }

        if (ioctl(m_master_fd, TIOSCTTY)) {
            perror("terminal (fork): ioctl(TIOSCTTY)");
            _exit(1);
        }

        signal(SIGTTOU, SIG_DFL);

        dup2(slave_fd, STDIN_FILENO);
        dup2(slave_fd, STDOUT_FILENO);
        dup2(slave_fd, STDERR_FILENO);

        close(slave_fd);
        close(m_master_fd);

        putenv((char*) "TERM=xterm");
        execl("/bin/sh", "sh", NULL);
        _exit(127);
    }

    tcsetpgrp(m_master_fd, m_child_pid);
}

PsuedoTerminal::~PsuedoTerminal() {
    kill(m_child_pid, SIGHUP);
    close(m_master_fd);
}
