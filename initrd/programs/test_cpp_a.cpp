#include <iostream>
#include <vector>

std::vector ints { 0, 1, 2 };

int main() {
    double x = 1.123;
    std::cout << x << std::endl;

    std::cout << ints.at(4) << std::endl;

    return 0;
}
