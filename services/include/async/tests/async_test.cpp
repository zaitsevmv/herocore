#include <iostream>

#include "../async_task.hpp"
#include "../../logger/logger.hpp"

int main() {
    std::cout << "abc" << std::endl;
    std::cerr << "abc" << std::endl;
    SET_LOG_FILE("test_log");

    LOG_DEBUG() << "abc";

    return 0;
}
