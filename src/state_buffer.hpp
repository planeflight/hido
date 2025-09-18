#ifndef STATEBUFFER_HPP
#define STATEBUFFER_HPP

#include <cstdint>
#include <deque>

template <typename T>
class StateBuffer {
  public:
    StateBuffer() = default;
    size_t size() const {
        return buffer.size();
    }

    // force second element to be after render_time
    // shorten such that a=buffer[0] < render_time < b=buffer[1]
    void remove_unused(uint64_t render_time) {
        while (buffer.size() > 2 && buffer[1].header.timestamp <= render_time) {
            buffer.pop_front();
        }
    }

    T &operator[](size_t idx) {
        return buffer[idx];
    }
    const T &operator[](size_t idx) const {
        return buffer[idx];
    }
    void push_back(const T &t) {
        buffer.push_back(t);
    }

  private:
    std::deque<T> buffer;
};

#endif // STATEBUFFER_HPP
