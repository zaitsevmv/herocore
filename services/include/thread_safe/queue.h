#pragma once

#include <cstddef>
#include <mutex>
#include <optional>
#include <queue>
#include <shared_mutex>

template<typename T>
class TQueueSafe {
public:
    std::queue<T> Clone() {
        return q_;
    }

    void Clear() {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        std::queue<T> newQ;
        q_.swap(newQ);
    }

    void Push(const T& value) {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        q_.push(value);
    }

    void Push(T&& value) {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        q_.push(std::move(value));
    }

    T Pop() {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        auto value = q_.front();
        q_.pop();
        return value;
    }

    std::optional<T> TryPop() {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        if (q_.empty()) {
            return std::nullopt;
        }
        auto value = q_.front();
        q_.pop();
        return value;
    }

    T Front() {
        std::shared_lock<std::shared_mutex> lock(mutex_);
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