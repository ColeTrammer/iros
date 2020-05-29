#include <assert.h>
#include <clipboard/connection.h>
#include <clipboard_server/message.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace Clipboard {

void Connection::initialize() {}

Connection& Connection::the() {
    static Connection* s_connection;
    if (!s_connection) {
        s_connection = new Connection;
    }
    return *s_connection;
}

static int open_connection() {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }

    sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "/tmp/.clipboard_server.socket");
    if (connect(fd, (sockaddr*) &addr, sizeof(sockaddr_un)) < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

bool Connection::set_clipboard_contents_to_text(const String& text) {
    int fd = open_connection();
    if (fd < 0) {
        return false;
    }

    auto request = ClipboardServer::Message::SetContentsRequest::create("text/plain", text.string(), text.size() + 1);
    if (write(fd, request.get(), request->total_size()) != static_cast<ssize_t>(request->total_size())) {
        close(fd);
        return false;
    }

    char buf[2048];
    if (read(fd, buf, sizeof(buf)) <= 0) {
        close(fd);
        return false;
    }

    close(fd);
    return reinterpret_cast<ClipboardServer::Message*>(buf)->data.set_contents_response.success;
}

Maybe<String> Connection::get_clipboard_contents_as_text() {
    int fd = open_connection();
    if (fd < 0) {
        return {};
    }

    auto request = ClipboardServer::Message::GetContentsRequest::create("text/plain");
    if (write(fd, request.get(), request->total_size()) != static_cast<ssize_t>(request->total_size())) {
        close(fd);
        return {};
    }

    char buf[0x8000];
    if (read(fd, buf, sizeof(buf)) <= 0) {
        close(fd);
        return {};
    }

    auto& response_message = *reinterpret_cast<ClipboardServer::Message*>(buf);
    auto& response = response_message.data.get_contents_response;
    if (!response.success) {
        return {};
    }

    assert((reinterpret_cast<char*>(response.data))[response_message.data_len - sizeof(ClipboardServer::Message::GetConentsResponse)] ==
           '\0');
    return { String(response.data) };
}

}
