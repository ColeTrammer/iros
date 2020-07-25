#include <ext/mapped_file.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace Ext {

UniquePtr<MappedFile> MappedFile::create(const String& path, int prot, int type) {
    int fd = open(path.string(), O_RDONLY);
    if (fd == -1) {
        perror("open");
        return nullptr;
    }

    struct stat st;
    if (fstat(fd, &st)) {
        perror("fstat");
        close(fd);
        return nullptr;
    }

    uint8_t* raw_data = (uint8_t*) mmap(nullptr, st.st_size, prot, type, fd, 0);
    if (raw_data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return nullptr;
    }

    if (close(fd)) {
        perror("close");
        munmap(raw_data, st.st_size);
        return nullptr;
    }

    return make_unique<MappedFile>(raw_data, st.st_size);
}

MappedFile::~MappedFile() {
    munmap(m_data, m_size);
}

}
