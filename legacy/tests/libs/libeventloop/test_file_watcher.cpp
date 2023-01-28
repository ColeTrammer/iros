#include <eventloop/event_loop.h>
#include <eventloop/file_watcher.h>
#include <eventloop/timer.h>
#include <test/test.h>
#include <unistd.h>

TEST(file_watcher, basic) {
    FILE* file = fopen("/tmp/test_file_watcher", "w");
    setvbuf(file, nullptr, _IONBF, 0);

    {
        auto loop = App::EventLoop {};

        auto file_watcher = App::FileWatcher::create(nullptr);
        EXPECT(file_watcher->watch("/tmp/test_file_watcher"));

        int count = 0;
        file_watcher->on<App::PathChangeEvent>({}, [&](auto& event) {
            EXPECT_EQ(event.path(), "/tmp/test_file_watcher");
            if (++count >= 3) {
                loop.set_should_exit(true);
                return;
            }
            fputc('a', file);
        });

        fputc('a', file);
        loop.enter();

        EXPECT_EQ(count, 3);

        EXPECT(file_watcher->unwatch("/tmp/test_file_watcher"));
        auto timer = App::Timer::create_single_shot_timer(nullptr, 100);
        timer->on<App::TimerEvent>({}, [&](auto&) {
            loop.set_should_exit(true);
        });

        fputc('a', file);
        loop.set_should_exit(false);
        loop.enter();
        EXPECT_EQ(count, 3);
    }

    fclose(file);
    unlink("/tmp/test_file_watcher");
}

TEST(file_watcher, remove) {
    FILE* file = fopen("/tmp/test_file_watcher", "w");
    setvbuf(file, nullptr, _IONBF, 0);

    {
        auto loop = App::EventLoop {};

        auto file_watcher = App::FileWatcher::create(nullptr);
        EXPECT(file_watcher->watch("/tmp/test_file_watcher"));

        bool did_remove = false;
        file_watcher->on<App::PathRemovedEvent>({}, [&](auto& event) {
            EXPECT_EQ(event.path(), "/tmp/test_file_watcher");
            did_remove = true;
            loop.set_should_exit(true);
        });

        unlink("/tmp/test_file_watcher");
        fclose(file);

        loop.enter();
        EXPECT(did_remove);
    }
}
