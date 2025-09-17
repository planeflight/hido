#include "util.hpp"

namespace timer {
std::chrono::time_point<std::chrono::high_resolution_clock> start;

void init() {
    start = std::chrono::high_resolution_clock::now();
}

std::chrono::time_point<std::chrono::high_resolution_clock> get_start() {
    return start;
}
} // namespace timer
