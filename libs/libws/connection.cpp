#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>
#include <window_server/connection.h>
#include <window_server/message.h>
#include <window_server/window.h>

namespace WindowServer {

Connection::Connection() : m_fd(socket(AF_UNIX, SOCK_STREAM, 0)) {
    assert(m_fd != -1);

    sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "/tmp/.window_server.socket");

    assert(connect(m_fd, (sockaddr*) &addr, sizeof(sockaddr_un)) == 0);
}

Connection::~Connection() {
    close(m_fd);
}

SharedPtr<Window> Connection::create_window(int x, int y, int width, int height, const String& name) {
    auto create_message = WindowServer::Message::CreateWindowRequest::create(x, y, width, height, name);
    assert(write(m_fd, create_message.get(), create_message->total_size()) != -1);

    uint8_t message_buffer[4096];
    auto* message = reinterpret_cast<Message*>(message_buffer);
    read(m_fd, message_buffer, 4096);
    assert(message->type == Message::Type::CreateWindowResponse);

    Message::CreateWindowResponse& created_data = message->data.create_window_response;
    return Window::construct(Rect(x, y, width, height), created_data, *this);
}

void Connection::send_swap_buffer_request(wid_t wid) {
    auto swap_buffer_request = WindowServer::Message::SwapBufferRequest::create(wid);
    assert(write(m_fd, swap_buffer_request.get(), swap_buffer_request->total_size()) != -1);
}

void Connection::setup_timer() {
    // struct sigaction act;
    // sigfillset(&act.sa_mask);
    // act.sa_flags = SA_SIGINFO;
    // act.sa_sigaction = [](int, siginfo_t* info, void*) {
    //     Connection* connection = static_cast<Connection*>(info->si_value.sival_ptr);
    //     connection->windows().for_each([](auto* window) {
    //         window->draw();
    //     });
    // };
    // sigaction(SIGRTMIN, &act, nullptr);

    // sigevent ev;
    // ev.sigev_notify = SIGEV_SIGNAL;
    // ev.sigev_value.sival_ptr = static_cast<void*>(this);
    // ev.sigev_signo = SIGRTMIN;

    // timer_t timerid;
    // if (timer_create(CLOCK_MONOTONIC, &ev, &timerid)) {
    //     perror("timer_create");
    //     exit(1);
    // }

    // struct itimerspec spec;
    // spec.it_interval.tv_sec = 2;
    // spec.it_interval.tv_nsec = 0; // 10 FPS
    // spec.it_value = spec.it_interval;
    // if (timer_settime(timerid, 0, &spec, nullptr)) {
    //     perror("timer_settime");
    //     exit(1);
    // }
}

}