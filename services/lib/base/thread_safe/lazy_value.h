#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <optional>

template<typename T>
class Lazy {
public:
    Lazy(std::function<T()>&& gen) 
        : Generator_(gen)
    {}

    T& Get() {
        if (IsSet_.test(std::memory_order_relaxed)) {
            return *Value_;
        }
        {
            std::lock_guard<std::mutex> lock(GenMutex_);
            if (IsSet_.test(std::memory_order_relaxed)) {
                return *Value_;
            }
            Value_ = Generator_();
            IsSet_.test_and_set(std::memory_order::consume);
        }
        return *Value_;
    }

private:
    std::function<T()> Generator_;
    std::atomic_flag IsSet_;
    std::optional<T> Value_;
    std::mutex GenMutex_;
};
