#pragma once

#include <cstddef>
#include <mutex>
#include <queue>
#include <shared_mutex>

template<typename T>
class TQueueSafe {
public:
    std::queue<T> Clone() {
        return q_;
    }

    void Push(const T& value) {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        q_.push(value);
    }

    T Pop() {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        if (q_.empty()) {
            return T();
        }
        auto value = q_.front();
        q_.pop();
        return value;
    }

    T Front() {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        if (q_.empty()) {
            return T();
        }
        return q_.front();
    }

    size_t Size() {
        return q_.size();
    }

    bool Empty() {
        return q_.empty();
    }

private:
    std::queue<T> q_;
    std::shared_mutex mutex_;
};