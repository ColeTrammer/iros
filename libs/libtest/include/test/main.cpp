#include <test/test_manager.h>

int main(int argc, char** argv) {
    return Test::TestManager::the().do_main(argc, argv);
}
