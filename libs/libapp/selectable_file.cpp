#include <app/selectable_file.h>
#include <fcntl.h>
#include <unistd.h>

namespace App {

SelectableFile::SelectableFile(const String& path, int oflags, mode_t mode) {
    int fd = open(path.string(), oflags, mode);
    if (fd == -1) {
        return;
    }

    set_fd(fd);
    set_selected_events((oflags & O_RDONLY) ? NotifyWhen::Readable
                                            : (oflags & O_WRONLY) ? NotifyWhen::Writeable
                                                                  : (oflags & O_RDWR) ? (NotifyWhen::Readable | NotifyWhen::Writeable) : 0);
    enable_notifications();
}

SelectableFile::~SelectableFile() {
    if (valid()) {
        close(fd());
    }
}

}
