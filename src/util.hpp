#ifndef HIDO_UTIL_HPP
#define HIDO_UTIL_HPP

template <typename T>
int sign(T val) {
    return (T(0) < val) - (val < T(0));
}
#endif // HIDO_UTIL_HPP
