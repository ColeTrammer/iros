#include "dius/sync_file.h"

#include "di/assert/prelude.h"
#include "di/container/algorithm/prelude.h"
#include "di/function/prelude.h"
#include "dius/system/system_call.h"

namespace dius {
static auto sys_read(int fd, di::Span<byte> data) -> di::Expected<usize, di::GenericCode> {
    return system::system_call<usize>(system::Number::read, fd, data.data(), data.size());
}

static auto sys_write(int fd, di::Span<byte const> data) -> di::Expected<usize, di::GenericCode> {
    return system::system_call<usize>(system::Number::write, fd, data.data(), data.size());
}

static auto sys_pread(int fd, u64 offset, di::Span<byte> data) -> di::Expected<usize, di::GenericCode> {
    return system::system_call<usize>(system::Number::pread, fd, data.data(), data.size(), offset);
}

static auto sys_pwrite(int fd, u64 offset, di::Span<byte const> data) -> di::Expected<usize, di::GenericCode> {
    return system::system_call<usize>(system::Number::pwrite, fd, data.data(), data.size(), offset);
}

static auto sys_close(int fd) -> di::Expected<void, di::GenericCode> {
    return system::system_call<int>(system::Number::close, fd) % di::into_void;
}

static auto sys_open(di::PathView path, int flags, u16 create_mode) -> di::Expected<int, di::GenericCode> {
    auto raw_data = path.data();
    char null_terminated_string[4097];
    ASSERT_LT(raw_data.size(), sizeof(null_terminated_string) - 1);

    di::copy(raw_data, null_terminated_string);
    null_terminated_string[raw_data.size()] = '\0';

    return system::system_call<int>(system::Number::openat, AT_FDCWD, null_terminated_string, flags, create_mode);
}

static auto sys_ftruncate(int fd, u64 size) -> di::Expected<void, di::GenericCode> {
    return system::system_call<int>(system::Number::ftruncate, fd, size) % di::into_void;
}

static auto sys_mmap(void* addr, usize length, Protection prot, MapFlags flags, int fd, u64 offset)
    -> di::Expected<byte*, di::GenericCode> {
    return system::system_call<byte*>(system::Number::mmap, addr, length, prot, flags, fd, offset);
}
static auto sys_ioctl(int fd, int code, void* arg) -> di::Expected<void, di::GenericCode> {
    return system::system_call<int>(system::Number::ioctl, fd, code, arg) % di::into_void;
}

static auto sys_grantpt(int fd) -> di::Expected<void, di::GenericCode> {
    u32 pty_number = 0;
    return sys_ioctl(fd, TIOCGPTN, &pty_number);
}

static auto sys_unlockpt(int fd) -> di::Expected<void, di::GenericCode> {
    int unlock = 0;
    return sys_ioctl(fd, TIOCSPTLCK, &unlock);
}

static auto sys_tcgetattr(int fd) -> di::Expected<termios, di::GenericCode> {
    termios result;
    TRY(sys_ioctl(fd, TCGETS, &result));
    return result;
}

static auto sys_tcsetattr(int fd, termios const& termios) -> di::Expected<void, di::GenericCode> {
    return sys_ioctl(fd, TCSETS, di::voidify(&termios));
}

auto SyncFile::close() -> di::Expected<void, di::GenericCode> {
    auto owned = di::exchange(m_owned, Owned::No);
    auto fd = di::exchange(m_fd, -1);

    if (owned == Owned::Yes && fd != -1) {
        return sys_close(fd);
    }
    return {};
}

auto SyncFile::read_some(di::Span<byte> data) const -> di::Expected<usize, di::GenericCode> {
    return sys_read(m_fd, data);
}

auto SyncFile::read_some(u64 offset, di::Span<byte> data) const -> di::Expected<usize, di::GenericCode> {
    return sys_pread(m_fd, offset, data);
}

auto SyncFile::write_some(di::Span<byte const> data) const -> di::Expected<usize, di::GenericCode> {
    return sys_write(m_fd, data);
}

auto SyncFile::write_some(u64 offset, di::Span<byte const> data) const -> di::Expected<usize, di::GenericCode> {
    return sys_pwrite(m_fd, offset, data);
}

auto SyncFile::resize_file(u64 new_size) const -> di::Expected<void, di::GenericCode> {
    return sys_ftruncate(file_descriptor(), new_size);
}

auto SyncFile::map(u64 offset, usize size, Protection protection, MapFlags flags) const
    -> di::Expected<MemoryRegion, di::GenericCode> {
    auto* base = TRY(sys_mmap(nullptr, size, protection, flags, m_fd, offset));
    return MemoryRegion(di::Span { base, size });
}

auto SyncFile::set_tty_window_size(tty::WindowSize size) -> di::Expected<void, di::GenericCode> {
    ::winsize ws {};
    ws.ws_row = size.rows;
    ws.ws_col = size.cols;
    ws.ws_xpixel = size.pixel_width;
    ws.ws_ypixel = size.pixel_height;
    return sys_ioctl(file_descriptor(), TIOCSWINSZ, &ws);
}

auto SyncFile::get_tty_window_size() -> di::Expected<tty::WindowSize, di::GenericCode> {
    ::winsize ws {};
    TRY(sys_ioctl(file_descriptor(), TIOCGWINSZ, &ws));
    return tty::WindowSize { ws.ws_row, ws.ws_col, ws.ws_xpixel, ws.ws_ypixel };
}

auto SyncFile::get_psuedo_terminal_path() -> di::Expected<di::Path, di::GenericCode> {
    u32 pty_number = 0;
    TRY(sys_ioctl(file_descriptor(), TIOCGPTN, &pty_number));

    auto result = "/dev/pts"_p;
    auto pty = di::to_string(pty_number) | di::transform([](c32 code_point) {
                   return char(code_point);
               }) |
               di::to<di::TransparentString>();
    result /= pty;
    return result;
}

auto SyncFile::enter_raw_mode() -> di::Expected<RawModeToken, di::GenericCode> {
    auto original = TRY(sys_tcgetattr(file_descriptor()));
    auto with_raw_mode = original;

    with_raw_mode.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    with_raw_mode.c_oflag &= ~(OPOST);
    with_raw_mode.c_cflag |= (CS8);
    with_raw_mode.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    TRY(sys_tcsetattr(file_descriptor(), with_raw_mode));

    return RawModeToken(di::make_function<void()>([fd = file_descriptor(), original] {
        (void) sys_tcsetattr(fd, original);
    }));
}

static auto open_mode_flags(OpenMode open_mode) -> int {
    switch (open_mode) {
        case OpenMode::Readonly:
            return O_RDONLY;
        case OpenMode::WriteNew:
            return O_WRONLY | O_EXCL | O_CREAT;
        case OpenMode::WriteClobber:
            return O_WRONLY | O_TRUNC | O_CREAT;
        case OpenMode::ReadWrite:
            return O_RDWR;
        case OpenMode::AppendOnly:
            return O_WRONLY | O_APPEND | O_CREAT;
        case OpenMode::ReadWriteClobber:
            return O_RDWR | O_TRUNC | O_CREAT;
        case OpenMode::AppendReadWrite:
            return O_RDWR | O_APPEND | O_CREAT;
        default:
            di::unreachable();
    }
}

auto open_sync(di::PathView path, OpenMode open_mode, u16 create_mode) -> di::Expected<SyncFile, di::GenericCode> {
    auto flags = open_mode_flags(open_mode);
    auto fd = TRY(sys_open(path, flags, create_mode));
    return SyncFile { SyncFile::Owned::Yes, fd };
}

auto open_psuedo_terminal_controller(OpenMode open_mode, tty::WindowSize size)
    -> di::Expected<SyncFile, di::GenericCode> {
    auto flags = open_mode_flags(open_mode);
    auto fd = TRY(sys_open("/dev/ptmx"_pv, flags, 0));
    auto result = SyncFile { SyncFile::Owned::Yes, fd };

    // Set window size, and call grantpt() + unlockpt().
    TRY(sys_grantpt(result.file_descriptor()));
    TRY(sys_unlockpt(result.file_descriptor()));
    TRY(result.set_tty_window_size(size));

    return result;
}

auto open_tempory_file() -> di::Expected<SyncFile, di::GenericCode> {
    auto fd = TRY(sys_open("/tmp"_pv, O_TMPFILE | O_RDWR, 0666));
    return SyncFile { SyncFile::Owned::Yes, fd };
}
}
