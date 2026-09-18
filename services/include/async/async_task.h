#pragma once

#include <concepts>
#include <coroutine>
#include <exception>
#include <memory>

#include <include/async/base_awaiters.h>
#include <include/async/timed_runner.h>
#include <include/executor/executor.h>

namespace NAsync {

template<std::movable T>
class TPromiseType;

template<std::movable T, std::derived_from<TPromiseType<T>> P = TPromiseType<T>>
class TAsyncTask {
public:
    using THandle = std::coroutine_handle<P>;
    using promise_type = P;

    TAsyncTask(THandle h)
        :Handle_(h) {}

    TAsyncTask(const TAsyncTask&) = delete;
    TAsyncTask& operator=(const TAsyncTask&) = delete;

    TAsyncTask(TAsyncTask&& o) noexcept
        : Handle_(std::exchange(o.Handle_, nullptr)) {}
    TAsyncTask& operator=(TAsyncTask&& o) noexcept {
        if (this != &o) {
            if (Handle_) Handle_.destroy();
            Handle_ = std::exchange(o.Handle_, nullptr);
        }
        return *this;
    }

    // ~TAsyncTask() {
    //     if (Handle_) {
    //         Handle_.destroy(); 
    //     }
    // }

    const P& GetPromise() const {
        return Handle_.promise();
    }

    P& GetPromise() {
        return Handle_.promise();
    }

    auto& Run() {
        Handle_.promise().Resume();
        return *this;
    }

    TTaskAwaiter<THandle, T> operator co_await() const noexcept {
        return TTaskAwaiter<THandle, T>(Handle_);
    }

private:
    THandle Handle_ = nullptr;
};

template<std::movable T>
class TPromiseType : public IResumable {
public:
    void Resume() override {
        auto h = std::coroutine_handle<TPromiseType>::from_promise(*this);
        h.resume();
    }

    auto initial_suspend() {
        return std::suspend_always();
    }

    auto final_suspend() noexcept {
        struct TFinalAwaiter {
            bool await_ready() noexcept { return false; }
            std::coroutine_handle<> await_suspend(std::coroutine_handle<TPromiseType> h) noexcept {
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
        return TAsyncTask<T, TPromiseType<T>>(std::coroutine_handle<TPromiseType<T>>::from_promise(*this));
    }

    void return_value(T value) {
        TaskResult_ = std::make_shared<T>(std::move(value));
    }

    void unhandled_exception() {
        Exception_ = std::current_exception();
    }

    void SetContinuation(std::coroutine_handle<> continuation) noexcept {
        Continuation_ = std::move(continuation);
    }

    auto GetTaskResult() {
        return *TaskResult_;
    }

    auto GetException() noexcept {
        return Exception_;
    }

    ~TPromiseType() = default;

private:
    std::shared_ptr<T> TaskResult_ = nullptr;
    std::coroutine_handle<> Continuation_;
    std::exception_ptr Exception_;
};

template<typename T>
requires std::movable<T>
class TExecutorPromiseType : public TPromiseType<T> {
public:
    void Resume() override {
        auto h = std::coroutine_handle<TExecutorPromiseType>::from_promise(*this);
        if (Executor_ == nullptr) {
            h.resume();
            return;
        }
        Executor_->Append([h](){
            h.resume();
        });
    }

    auto get_return_object() {
        return TAsyncTask<T, TExecutorPromiseType<T>>(std::coroutine_handle<TExecutorPromiseType>::from_promise(*this));
    }

    void SetExecutor(IExecutorPtr executor) noexcept {
        Executor_ = std::move(executor);
    }

    ~TExecutorPromiseType() = default;

private:
    IExecutorPtr Executor_ = nullptr;
};

template<std::movable T, std::derived_from<TPromiseType<T>> P>
auto& Run(TAsyncTask<T, P>& task) {
    task.Run();
    return task;
}

template<std::movable T>
auto& RunWith(TAsyncTask<T, TExecutorPromiseType<T>>& task, IExecutorPtr executor) {
    task.GetPromise().SetExecutor(std::move(executor));
    task.Run();
    return task;
}

enum class ETimedRunStatus : uint {
    Ok = 0u, Timedout = 1u
};
template<std::movable T, std::derived_from<TPromiseType<T>> P>
using TTimedAsyncTask = std::variant<TAsyncTask<T, P>, ETimedRunStatus>;

template<std::movable T, std::derived_from<TPromiseType<T>> P>
auto& WithTimeout(TAsyncTask<T, P>& task, TTimedRunnerPtr timedRunner, const TTimePointType deadline) {
    timedRunner->AddToRunner(
        []() {

        }, deadline
    );
    return task;
}

template<std::movable T, std::derived_from<TPromiseType<T>> P>
auto& WithTimeout(TAsyncTask<T, P>& task, TTimedRunnerPtr timedRunner, const TDurationType duration) {
    return WithTimeout(task, timedRunner, TTimePointType::clock::now() + duration);
}

} // namespace NAsync
