#include <dius/print.h>
#include <dius/system/prelude.h>

extern "C" int main() {
    auto* x = new int;
    *x = 42;

    auto* y = new int;
    *y = 42;
    for (unsigned int i = 0; i < 3; i++) {
        dius::println("Spawning task {}."_sv, i);

        auto args = di::single("/test_userspace"_tsv) | di::transform(di::to_owned) | di::to<di::Vector>();
        (void) dius::system::Process(di::move(args)).spawn_and_wait();
    }

    {
        auto args = di::single("/test_read"_tsv) | di::transform(di::to_owned) | di::to<di::Vector>();
        (void) dius::system::Process(di::move(args)).spawn_and_wait();
    }

    dius::println("Finished."_sv);
    return 0;
}
