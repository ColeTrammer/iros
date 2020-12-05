#include <assert.h>
#include <eventloop/unix_socket.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace App {

SharedPtr<UnixSocket> UnixSocket::create_from_fd(SharedPtr<Object> parent, int fd, bool nonblocking) {
    return UnixSocket::create(move(parent), fd, nonblocking);
}

SharedPtr<UnixSocket> UnixSocket::create_connection(SharedPtr<Object> parent, const String& path) {
    int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        return nullptr;
    }

    sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    assert(path.size() + 1 < sizeof(addr.sun_path));
    strcpy(addr.sun_path, path.string());
    if (connect(fd, (const struct sockaddr*) &addr, sizeof(addr)) < 0) {
        close(fd);
        return nullptr;
    }

    return UnixSocket::create(move(parent), fd, true);
}

UnixSocket::UnixSocket(int fd, bool nonblocking) : m_nonblocking(nonblocking) {
    set_fd(fd);
    set_selected_events(NotifyWhen::Readable);
    enable_notifications();
}

UnixSocket::~UnixSocket() {
    close(fd());
}

void UnixSocket::set_nonblocking(bool b) {
    if (b == m_nonblocking) {
        return;
    }

    m_nonblocking = b;
    int flags = fcntl(fd(), F_GETFL);
    if (flags != -1) {
        if (b) {
            flags |= O_NONBLOCK;
        } else {
            flags &= ~O_NONBLOCK;
        }
        fcntl(fd(), F_SETFL, flags);
    }
}

void UnixSocket::disconnect() {
    if (on_disconnect) {
        on_disconnect(*this);
    }
}

void UnixSocket::notify_readable() {
    if (on_ready_to_read) {
        on_ready_to_read(*this);
    }
}

}
