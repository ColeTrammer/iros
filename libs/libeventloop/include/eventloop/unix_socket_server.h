#include <eventloop/selectable.h>
#include <eventloop/unix_socket.h>
#include <liim/function.h>
#include <liim/string.h>

namespace App {

class UnixSocketServer final : public Selectable {
    APP_OBJECT(UnixSocketServer)

public:
    UnixSocketServer(const String& bind_path);
    virtual ~UnixSocketServer();

    Function<void()> on_ready_to_accept;

    SharedPtr<UnixSocket> accept();

private:
    virtual void notify_readable() override;
};

}
