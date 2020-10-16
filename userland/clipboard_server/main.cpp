#include <assert.h>
#include <clipboard_server/message.h>
#include <eventloop/event_loop.h>
#include <eventloop/unix_socket.h>
#include <eventloop/unix_socket_server.h>
#include <liim/string.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <syslog.h>
#include <unistd.h>

int main() {
    using namespace ClipboardServer;

    mode_t mask = umask(0002);
    auto server = App::UnixSocketServer::create(nullptr, "/tmp/.clipboard_server.socket");
    umask(mask);

    UniquePtr<char> clipboard_contents;
    size_t clipboard_contents_size = 0;
    String clipboard_type;

    Vector<SharedPtr<App::UnixSocket>> clients;

    server->on_ready_to_accept = [&] {
        SharedPtr<App::UnixSocket> client;
        while ((client = server->accept())) {
            clients.add(client);
            client->on_disconnect = [&clients, &server](auto& client) {
                auto ptr = client.shared_from_this();
                clients.remove_element(ptr);
                server->remove_child(ptr);
            };
            client->on_ready_to_read = [&clients, &clipboard_contents, &clipboard_contents_size, &clipboard_type, &server](auto& client) {
                char buf[0x4000];
                ssize_t ret = read(client.fd(), buf, sizeof(buf));
                if (ret <= 0) {
                    if (ret < 0) {
                        perror("clipboard_server: read");
                    }
                    auto ptr = client.shared_from_this();
                    clients.remove_element(ptr);
                    server->remove_child(ptr);
                    return;
                }

                auto send_message = [&](Message& message, int fd) {
                    assert(write(fd, &message, message.total_size()) == static_cast<ssize_t>(message.total_size()));
                };

                auto* message = reinterpret_cast<Message*>(buf);
                switch (message->type) {
                    case Message::Type::GetContentsRequest: {
                        auto& request = message->data.get_contents_request;

                        UniquePtr<Message> response;
                        if (!clipboard_contents || strcmp(request.content_type, clipboard_type.string()) != 0) {
                            response = Message::GetConentsResponse::create(nullptr, 0);
                        } else {
                            response = Message::GetConentsResponse::create(clipboard_contents.get(), clipboard_contents_size);
                        }
                        send_message(*response, client.fd());
                        break;
                    }
                    case Message::Type::SetContentsRequest: {
                        auto& request = message->data.set_contents_request;
                        clipboard_contents_size = request.data_length;
                        clipboard_type = String(request.content_type());
                        clipboard_contents = UniquePtr<char>((char*) malloc(request.data_length));
                        memcpy(clipboard_contents.get(), request.data(), request.data_length);

                        syslog(
                            LOG_INFO, "set clipboard content: type='%s' data='%s'", clipboard_type.string(),
                            String(StringView(clipboard_contents.get(), clipboard_contents.get() + clipboard_contents_size - 1)).string());

                        send_message(*Message::SetContentsResponse::create(true), client.fd());
                        break;
                    }
                    default:
                        syslog(LOG_WARNING, "Invalid message to clipboard server");
                        break;
                }
            };
            client = nullptr;
        }
    };

    App::EventLoop loop;
    loop.enter();
    return 0;
}
