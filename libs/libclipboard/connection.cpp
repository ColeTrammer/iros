#ifdef __os_2__

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

#else

#include <X11/Xlib.h>
#include <assert.h>
#include <clipboard/connection.h>
#include <errno.h>
#include <pthread.h>
#include <sys/select.h>

namespace Clipboard {

static int s_x_fd;
static Display* s_display;
static int s_screen;
static Window s_root;
static Window s_target_window;
static Atom s_target_property;
static Atom s_selection_atom;
static Atom s_text_type_atom;
static Atom s_incr_atom;

static pthread_t s_thread_id;
static pthread_mutex_t s_clipboard_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t s_get_clipboard_condition = PTHREAD_COND_INITIALIZER;
static Maybe<String> s_clipboard_contents;
static bool s_clipboard_contents_get_request_completed;

static void* x11_background_thread_start(void*) {
    auto handle_selection_notify_event = [](XSelectionEvent& event) {
        auto fail = []() {
            s_clipboard_contents_get_request_completed = true;
            s_clipboard_contents = {};
            pthread_cond_signal(&s_get_clipboard_condition);
        };

        if (event.property == None) {
            fail();
            return;
        }

        Atom type;
        int di;
        unsigned long dul, size;
        unsigned char* prop_ret = NULL;
        XGetWindowProperty(s_display, s_target_window, s_target_property, 0, 0, False, AnyPropertyType, &type, &di, &dul, &size, &prop_ret);
        XFree(prop_ret);

        if (type == s_incr_atom) {
            fail();
            return;
        }

        Atom da;
        XGetWindowProperty(s_display, s_target_window, s_target_property, 0, size, False, AnyPropertyType, &da, &di, &dul, &dul, &prop_ret);
        auto data = String((char*) prop_ret);
        XFree(prop_ret);
        XDeleteProperty(s_display, s_target_window, s_target_property);

        s_clipboard_contents_get_request_completed = true;
        s_clipboard_contents = { move(data) };
        pthread_cond_signal(&s_get_clipboard_condition);
    };

    for (;;) {
        fd_set set;
        FD_ZERO(&set);
        FD_SET(s_x_fd, &set);

        if (select(s_x_fd + 1, &set, nullptr, nullptr, nullptr) < 0) {
            assert(errno == EINTR);
            continue;
        }

        XEvent ev;
        if (XCheckIfEvent(
                s_display, &ev,
                [](Display*, XEvent*, XPointer) {
                    return True;
                },
                nullptr)) {

            pthread_mutex_lock(&s_clipboard_mutex);

            switch (ev.type) {
                case SelectionNotify: {
                    handle_selection_notify_event(ev.xselection);
                    break;
                }
                default:
                    break;
            }

            pthread_mutex_unlock(&s_clipboard_mutex);
        }
    }

    return nullptr;
}

void Connection::initialize() {
    s_display = XOpenDisplay(nullptr);
    assert(s_display);
    s_x_fd = XConnectionNumber(s_display);

    s_screen = DefaultScreen(s_display);
    s_root = RootWindow(s_display, s_screen);

    s_selection_atom = XInternAtom(s_display, "CLIPBOARD", False);
    s_text_type_atom = XInternAtom(s_display, "UTF8_STRING", False);
    s_incr_atom = XInternAtom(s_display, "INCR", False);

    s_target_window = XCreateSimpleWindow(s_display, s_root, -10, -10, 1, 1, 0, 0, 0);

    assert(!pthread_create(&s_thread_id, nullptr, x11_background_thread_start, nullptr));
}

Connection& Connection::the() {
    static Connection* s_connection;
    if (!s_connection) {
        s_connection = new Connection;
    }
    return *s_connection;
}

Maybe<String> Connection::get_clipboard_contents_as_text() {
    pthread_mutex_lock(&s_clipboard_mutex);

    s_clipboard_contents_get_request_completed = false;
    s_target_property = XInternAtom(s_display, "LIBCLIPBOARD", False);
    XConvertSelection(s_display, s_selection_atom, s_text_type_atom, s_target_property, s_target_window, CurrentTime);

    for (;;) {
        if (s_clipboard_contents_get_request_completed) {
            break;
        }
        pthread_cond_wait(&s_get_clipboard_condition, &s_clipboard_mutex);
    }

    auto result = move(s_clipboard_contents);

    pthread_mutex_unlock(&s_clipboard_mutex);
    return result;
}

bool Connection::set_clipboard_contents_to_text(const String&) {
    return false;
}

}

#endif /* __os_2__ */
