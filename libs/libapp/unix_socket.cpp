#include <app/unix_socket.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace App {

SharedPtr<UnixSocket> UnixSocket::create_from_fd(SharedPtr<Object> parent, int fd) {
    return UnixSocket::create(move(parent), fd);
}

SharedPtr<UnixSocket> UnixSocket::create_connection(SharedPtr<Object> parent, const String& path) {
    int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
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

    return create_from_fd(parent, fd);
}

UnixSocket::UnixSocket(int fd) {
    set_fd(fd);
    set_selected_events(NotifyWhen::Readable | NotifyWhen::Exceptional);
    enable_notifications();
}

UnixSocket::~UnixSocket() {
    close(fd());
}

void UnixSocket::notify_exceptional() {
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
