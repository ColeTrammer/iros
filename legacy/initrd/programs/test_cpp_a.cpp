#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

std::vector ints { 0, 1, 2 };

int main() {
    double x = 1.123;
    std::cout << x << std::endl;

    std::mutex m;
    auto t = std::thread([&] {
        std::lock_guard<std::mutex> guard(m);
        printf("T1\n");
    });

    std::this_thread::sleep_for(100ms);
    {
        std::lock_guard<std::mutex> guard(m);
        printf("T2\n");
    }
    t.join();

    std::cout << std::stoi("10") << std::endl;

    std::cout << ints.at(4) << std::endl;

    return 0;
}
