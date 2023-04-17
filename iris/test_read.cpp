#include <dius/sync_file.h>

extern "C" int main() {
    auto buffer = di::Array<di::Byte, 4096> {};

    auto file = *dius::open_sync("/data.txt"_pv, dius::OpenMode::Readonly);

    while (auto result = file.read_some({ buffer.data(), buffer.size() })) {
        if (*result > 0) {
            (void) dius::stdout.write_exactly({ buffer.data(), *result });
        } else {
            break;
        }
    }

    return 0;
}
