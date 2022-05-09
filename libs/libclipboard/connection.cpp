#include <assert.h>
#include <clipboard/connection.h>

#ifdef __iros__
#include <clipboard_server/message.h>

namespace Clipboard {
static IPC::Endpoint& endpoint() {
    static SharedPtr<IPC::Endpoint> endpoint;
    if (!endpoint) {
        endpoint = IPC::Endpoint::create(nullptr);
        endpoint->set_socket(App::UnixSocket::create_connection(endpoint.get(), "/tmp/.clipboard_server.socket"));
    }
    return *endpoint;
}

Connection& Connection::the() {
    static Connection s_connection;
    return s_connection;
}

bool Connection::set_clipboard_contents_to_text(const String& text) {
    Vector<char> raw_data(text.size());
    for (auto c : text) {
        raw_data.add(c);
    }

    auto response = endpoint().send_then_wait<ClipboardServer::Client::SetContentsRequest, ClipboardServer::Server::SetContentsResponse>({
        .type = "text/plain",
        .data = move(raw_data),
    });
    return response.has_value() && response.value().success;
}

Option<String> Connection::get_clipboard_contents_as_text() {
    auto response = endpoint().send_then_wait<ClipboardServer::Client::GetContentsRequest, ClipboardServer::Server::GetContentsResponse>(
        { .type = "text/plain" });
    if (!response.has_value()) {
        return {};
    }

    return { String(StringView(response.value().data.vector(), response.value().data.vector() + response.value().data.size())) };
}
}
#elif defined(USE_X11)
#include <X11/Xlib.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sys/select.h>

namespace Clipboard {
static int s_x_fd;
static Display* s_display;
static int s_screen;
static Window s_root;
static Window s_target_window;
static Atom s_target_property;
static Atom s_selection_atom;
static Atom s_targets_atom;
static Atom s_text_type_atoms[4];
static Atom s_incr_atom;
static Atom s_atom_atom;

static pthread_t s_thread_id;
static pthread_mutex_t s_clipboard_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t s_get_clipboard_condition = PTHREAD_COND_INITIALIZER;
static Option<String> s_clipboard_contents;
static bool s_clipboard_contents_get_request_completed;
static bool s_currently_own_clipboard;

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
        auto data = String(StringView((char*) prop_ret, size));
        XFree(prop_ret);
        XDeleteProperty(s_display, s_target_window, s_target_property);
        XFlush(s_display);

        s_clipboard_contents_get_request_completed = true;
        s_clipboard_contents = { move(data) };
        pthread_cond_signal(&s_get_clipboard_condition);
    };

    auto handle_selection_request_event = [](XSelectionRequestEvent& event) {
        auto fail = [&]() {
            XSelectionEvent ssev;
            ssev.type = SelectionNotify;
            ssev.requestor = event.requestor;
            ssev.selection = event.selection;
            ssev.target = event.target;
            ssev.property = None;
            ssev.time = event.time;
            XSendEvent(s_display, event.requestor, True, NoEventMask, (XEvent*) &ssev);
            XFlush(s_display);
        };

        if (event.property == None) {
            fail();
            return;
        }

        bool supported = event.target == s_targets_atom;
        for (auto supported_target : s_text_type_atoms) {
            if (event.target == supported_target) {
                supported = true;
                break;
            }
        }

        if (!supported) {
            fail();
            return;
        }

        if (event.target != s_targets_atom) {
            XChangeProperty(s_display, event.requestor, event.property, event.target, 8, PropModeReplace,
                            (unsigned char*) s_clipboard_contents.value().string(), s_clipboard_contents.value().size());
        } else {
            Atom possible_values[5] = { s_text_type_atoms[0], s_text_type_atoms[1], s_text_type_atoms[2], s_text_type_atoms[3],
                                        s_targets_atom };
            XChangeProperty(s_display, event.requestor, event.property, s_atom_atom, 32, PropModeReplace, (unsigned char*) possible_values,
                            sizeof(possible_values) / sizeof(Atom));
        }

        XSelectionEvent ssev;
        ssev.type = SelectionNotify;
        ssev.requestor = event.requestor;
        ssev.selection = event.selection;
        ssev.target = event.target;
        ssev.property = event.property;
        ssev.time = event.time;
        XSendEvent(s_display, event.requestor, True, NoEventMask, (XEvent*) &ssev);
        XFlush(s_display);
    };

    for (;;) {
        pthread_mutex_lock(&s_clipboard_mutex);

        XEvent ev;
        while (XCheckIfEvent(
            s_display, &ev,
            [](Display*, XEvent*, XPointer) {
                return True;
            },
            nullptr)) {

            switch (ev.type) {
                case SelectionNotify:
                    handle_selection_notify_event(ev.xselection);
                    break;
                case SelectionClear:
                    s_currently_own_clipboard = false;
                    break;
                case SelectionRequest:
                    handle_selection_request_event(ev.xselectionrequest);
                    break;
                default:
                    break;
            }
        }
        pthread_mutex_unlock(&s_clipboard_mutex);

        fd_set set;
        FD_ZERO(&set);
        FD_SET(s_x_fd, &set);

        timeval timeout { .tv_sec = 0, .tv_usec = 750'000 };
        if (select(FD_SETSIZE, &set, nullptr, nullptr, &timeout) < 0) {
            assert(errno == EINTR);
            continue;
        }
    }

    return nullptr;
}

static void initialize() {
    s_display = XOpenDisplay(nullptr);
    assert(s_display);
    s_x_fd = XConnectionNumber(s_display);

    s_screen = DefaultScreen(s_display);
    s_root = RootWindow(s_display, s_screen);

    s_selection_atom = XInternAtom(s_display, "CLIPBOARD", False);
    s_text_type_atoms[0] = XInternAtom(s_display, "UTF8_STRING", False);
    s_text_type_atoms[1] = XInternAtom(s_display, "TEXT", False);
    s_text_type_atoms[2] = XInternAtom(s_display, "STRING", False);
    s_text_type_atoms[3] = XInternAtom(s_display, "text/plain", False);
    s_targets_atom = XInternAtom(s_display, "TARGETS", False);
    s_incr_atom = XInternAtom(s_display, "INCR", False);
    s_atom_atom = XInternAtom(s_display, "ATOM", False);

    s_target_window = XCreateSimpleWindow(s_display, s_root, -10, -10, 1, 1, 0, 0, 0);

    assert(!pthread_create(&s_thread_id, nullptr, x11_background_thread_start, nullptr));
}

Connection& Connection::the() {
    static Connection* s_connection;
    if (!s_connection) {
        s_connection = new Connection;
        initialize();
    }
    return *s_connection;
}

Option<String> Connection::get_clipboard_contents_as_text() {
    pthread_mutex_lock(&s_clipboard_mutex);

    if (s_currently_own_clipboard) {
        auto result = s_clipboard_contents;
        pthread_mutex_unlock(&s_clipboard_mutex);
        return result;
    }

    s_clipboard_contents_get_request_completed = false;
    s_target_property = XInternAtom(s_display, "LIBCLIPBOARD", False);
    XConvertSelection(s_display, s_selection_atom, s_text_type_atoms[0], s_target_property, s_target_window, CurrentTime);
    XFlush(s_display);

    time_t start = time(nullptr);
    for (;;) {
        if (s_clipboard_contents_get_request_completed) {
            break;
        }
        timespec timeout { .tv_sec = 0, .tv_nsec = 750'000'000 };
        pthread_cond_timedwait(&s_get_clipboard_condition, &s_clipboard_mutex, &timeout);
        if (time(nullptr) - start >= 2) {
            pthread_mutex_unlock(&s_clipboard_mutex);
            return {};
        }
    }

    auto result = move(s_clipboard_contents);

    pthread_mutex_unlock(&s_clipboard_mutex);
    return result;
}

bool Connection::set_clipboard_contents_to_text(const String& text) {
    pthread_mutex_lock(&s_clipboard_mutex);

    s_clipboard_contents = { text };
    XSetSelectionOwner(s_display, s_selection_atom, s_target_window, CurrentTime);
    XFlush(s_display);

    s_currently_own_clipboard = true;

    pthread_mutex_unlock(&s_clipboard_mutex);
    return true;
}
}
#else
namespace Clipboard {
static String s_clipboard_text;

Connection& Connection::the() {
    static Connection s_connection;
    return s_connection;
}

bool Connection::set_clipboard_contents_to_text(const String& text) {
    s_clipboard_text = text;
    return true;
}

Option<String> Connection::get_clipboard_contents_as_text() {
    return s_clipboard_text;
}
}
#endif /* __iros__ */
