#ifndef UTIL_HPP
#define UTIL_HPP

#include <chrono>

template <typename T>
int sign(T val) {
    return (T(0) < val) - (val < T(0));
}
namespace timer {

std::chrono::time_point<std::chrono::high_resolution_clock> get_start();

void init();

/**
 * @return the time since the initialization of the engine in milliseconds
 */
template <typename T>
T get_time_millis() {
    return static_cast<T>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now() - get_start())
            .count() *
        0.001 * 0.001);
}

/**
 * @return the time since the initialization of the engine in seconds
 */
template <typename T>
T get_time() {
    return static_cast<T>(get_time_millis<double>() * 0.001);
}

} // namespace timer

#endif // UTIL_HPP
