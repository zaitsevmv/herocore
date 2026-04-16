#pragma once

#include <concepts>
#include <coroutine>
#include <exception>
#include <functional>
#include <memory>

namespace NAsync {

template<typename T>
requires std::movable<T>
class TAsyncTask {
public:
    struct promise_type;
    using TCallbackFunc = std::function<void(std::shared_ptr<T>, std::exception_ptr)>;
    using THandle = std::coroutine_handle<promise_type>;

    struct promise_type {
    public:
        auto initial_suspend() {
            return std::suspend_always();
        }

        auto final_suspend() noexcept {
            struct TFinalAwaiter {
                bool await_ready() noexcept { return false; }
                std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                    h.promise().AlertSubscribers();
                    if (h.promise().Continuation_ && !h.promise().Continuation_.done()) {
                        return h.promise().Continuation_;
                    }
                    return std::noop_coroutine();
                }
                void await_resume() noexcept {}
            };
            return TFinalAwaiter{};
        }

        auto get_return_object() {
            return TAsyncTask(THandle::from_promise(*this));
        }

        void return_value(T value) {
            TaskResult_ = std::make_shared<T>(std::move(value));
        }

        void unhandled_exception() {
            Exception_ = std::current_exception();
        }

        void AddCallback(std::function<void()> cb) {
            SubscriberCallbacks_.emplace_back(std::move(cb));
        }

        void AlertSubscribers() {
            for (auto& func: SubscriberCallbacks_) {
                func(TaskResult_, Exception_);
            }
        }

        std::shared_ptr<T> TaskResult_ = nullptr;
        std::coroutine_handle<> Continuation_;
        std::exception_ptr Exception_;
    private:
        std::vector<TCallbackFunc> SubscriberCallbacks_;
    };

    TAsyncTask(THandle h)
        :Handle_(h) {}

    TAsyncTask(const TAsyncTask&) = delete;
    TAsyncTask& operator=(const TAsyncTask&) = delete;
    TAsyncTask(TAsyncTask&&) = default;
    TAsyncTask& operator=(TAsyncTask&&) = default;

    void Run() {
        Handle_.resume();
    }

    void Subscribe(TCallbackFunc cb) {
        if (Handle_ && !Handle_.done()) {
            Handle_.promise().AddCallback(std::move(cb));
        } else if (Handle_) {
            cb(Handle_.promise().TaskResult_, Handle_.promise().Exception_);
        }
    }

    struct TTaskAwaiter {
        THandle TaskHandle = nullptr;

        operator bool() const {
            return TaskHandle != nullptr;
        }

        bool await_ready() const {
            return TaskHandle.done();
        }

        auto await_suspend(std::coroutine_handle<> handle) noexcept {
            TaskHandle.promise().Continuation_ = handle;
            return TaskHandle;
        }

        T await_resume() {
            if (TaskHandle.promise().Exception_) {
                std::rethrow_exception(TaskHandle.promise().Exception_);
            }
            return *TaskHandle.promise().TaskResult_;
        }
    };

    TTaskAwaiter operator co_await() const noexcept {
        return TTaskAwaiter(Handle_);
    }

private:
    THandle Handle_ = nullptr;
};

} // namespace NAsync
