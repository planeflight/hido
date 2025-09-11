#ifndef UTIL_HPP
#define UTIL_HPP

template <typename T>
int sign(T val) {
    return (T(0) < val) - (val < T(0));
}

#endif // UTIL_HPP
