#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>

#include "terminal.h"
#include "tty.h"
#include "vga_buffer.h"

void Terminal::load(VgaBuffer::GraphicsContainer& container) {
    m_buffer = make_unique<VgaBuffer>(container);
    m_tty = make_unique<TTY>(*m_buffer);

    m_mfd = posix_openpt(O_RDWR);
    assert(m_mfd != -1);

    m_pid = fork();
    if (m_pid == 0) {
        int sfd = open(ptsname(m_mfd), O_RDWR);
        assert(sfd != -1);

        if (setsid() < 0) {
            perror("setsid");
            _exit(1);
        }
        if (tcsetpgrp(m_mfd, getpid())) {
            perror("tcsetpgrp");
            _exit(1);
        }
        if (ioctl(m_mfd, TIOSCTTY)) {
            perror("ioctl(TIOSCTTY)");
            _exit(1);
        }
        signal(SIGTTOU, SIG_DFL);

        dup2(sfd, STDIN_FILENO);
        dup2(sfd, STDOUT_FILENO);
        dup2(sfd, STDERR_FILENO);

        close(sfd);
        close(m_mfd);

        putenv((char*) "TERM=xterm");
        execl("/bin/sh", "sh", NULL);
        _exit(127);
    } else if (m_pid == -1) {
        perror("fork");
        _exit(-1);
    }

    tcsetpgrp(m_mfd, m_pid);
}

Terminal::~Terminal() {
    if (is_loaded()) {
        if (m_pid != -1) {
            kill(m_pid, SIGHUP);
        }
        close(m_mfd);
    }
}