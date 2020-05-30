#include <assert.h>
#include <clipboard_server/message.h>
#include <liim/string.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int main() {
    using namespace ClipboardServer;

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "/tmp/.clipboard_server.socket");

    unlink(addr.sun_path);
    if (bind(fd, (const struct sockaddr*) &addr, sizeof(struct sockaddr_un)) == -1) {
        perror("bind");
        return 1;
    }

    if (listen(fd, 5) == -1) {
        perror("listen");
        return 1;
    }

    UniquePtr<char> clipboard_contents;
    size_t clipboard_contents_size = 0;
    String clipboard_type;

    auto send_message = [&](Message& message, int fd) {
        assert(write(fd, &message, message.total_size()) == static_cast<ssize_t>(message.total_size()));
    };

    char buf[0x8000];
    for (;;) {
        struct sockaddr_un addr;
        socklen_t addrlen = sizeof(struct sockaddr_un);
        int client_fd = accept(fd, (struct sockaddr*) &addr, &addrlen);
        if (client_fd == -1) {
            perror("accept");
            return 1;
        }

        ssize_t ret = read(client_fd, &buf, 2048);
        if (ret == -1) {
            perror("read");
            return 1;
        } else if (ret == 0) {
            close(client_fd);
            continue;
        }

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
                send_message(*response, client_fd);
                break;
            }
            case Message::Type::SetContentsRequest: {
                auto& request = message->data.set_contents_request;
                clipboard_contents_size = request.data_length;
                clipboard_type = String(request.content_type());
                clipboard_contents = UniquePtr<char>((char*) malloc(request.data_length));
                memcpy(clipboard_contents.get(), request.data(), request.data_length);

                fprintf(stderr, "set clipboard content: type='%s' data='%s'\n", clipboard_type.string(),
                        String(StringView(clipboard_contents.get(), clipboard_contents.get() + clipboard_contents_size - 1)).string());

                send_message(*Message::SetContentsResponse::create(true), client_fd);
                break;
            }
            default:
                fprintf(stderr, "Invalid message to clipboard server\n");
                break;
        }

        close(client_fd);
    }
}
