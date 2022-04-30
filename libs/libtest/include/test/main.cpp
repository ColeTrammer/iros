#include <test/test_manager.h>
#ifdef __iros__
#include <sys/iros.h>
#endif

int main(int argc, char** argv) {
#ifdef __iros__
    printf("\r\n");
    fflush(stdout);
#endif
    int result = Test::TestManager::the().do_main(argc, argv);
#ifdef __iros__
    poweroff();
    perror("poweroff");
#endif
    return result;
}
