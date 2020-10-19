#include <assert.h>
#include <eventloop/unix_socket_server.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace App {

UnixSocketServer::UnixSocketServer(const String& bind_path) {
    set_fd(socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0));
    if (fd() < 0) {
        return;
    }

    sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    assert(bind_path.size() + 1 < sizeof(addr.sun_path));
    strcpy(addr.sun_path, bind_path.string());
    assert(bind(fd(), (const sockaddr*) &addr, sizeof(addr)) == 0);
    assert(listen(fd(), 5) == 0);

    set_selected_events(NotifyWhen::Readable);
    enable_notifications();
}

SharedPtr<UnixSocket> UnixSocketServer::accept() {
    int fd = accept4(this->fd(), nullptr, nullptr, SOCK_CLOEXEC | SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (fd < 0) {
        return nullptr;
    }
    return UnixSocket::create_from_fd(shared_from_this(), fd, true);
}

UnixSocketServer::~UnixSocketServer() {
    if (fd() != -1) {
        close(fd());
    }
}

void UnixSocketServer::notify_readable() {
    if (on_ready_to_accept) {
        on_ready_to_accept();
    }
}

}
