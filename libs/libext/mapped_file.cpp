#include <ext/mapped_file.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace Ext {

UniquePtr<MappedFile> MappedFile::create(const String& path, int prot, int type) {
    int fd = open(path.string(), O_RDONLY);
    if (fd == -1) {
        return nullptr;
    }

    struct stat st;
    if (fstat(fd, &st)) {
        close(fd);
        return nullptr;
    }

    uint8_t* raw_data = (uint8_t*) mmap(nullptr, st.st_size, prot, type, fd, 0);
    if (raw_data == MAP_FAILED) {
        close(fd);
        return nullptr;
    }

    if (close(fd)) {
        munmap(raw_data, st.st_size);
        return nullptr;
    }

    return make_unique<MappedFile>(raw_data, st.st_size);
}

UniquePtr<MappedFile> MappedFile::create_with_shared_memory(const String& path, int prot) {
    int fd = shm_open(path.string(), O_RDONLY, 0);
    if (fd == -1) {
        return nullptr;
    }

    struct stat st;
    if (fstat(fd, &st)) {
        close(fd);
        return nullptr;
    }

    uint8_t* raw_data = (uint8_t*) mmap(nullptr, st.st_size, prot, MAP_SHARED, fd, 0);
    if (raw_data == MAP_FAILED) {
        close(fd);
        return nullptr;
    }

    if (close(fd)) {
        munmap(raw_data, st.st_size);
        return nullptr;
    }

    return make_unique<MappedFile>(raw_data, st.st_size);
}

MappedFile::~MappedFile() {
    munmap(m_data, m_size);
}

}
