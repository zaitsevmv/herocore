#pragma once

#include <concepts>
#include <coroutine>
#include <exception>
#include <memory>
#include <variant>

#include <include/async/timed_runner.h>
#include <include/executor/executor.h>

namespace NAsync {

template<std::movable T>
class TPromiseType;

template<std::movable T, std::derived_from<TPromiseType<T>> P>
class TAsyncTask;

template<std::movable T>
class TPromiseType {
public:
    void Resume() {
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
        return TAsyncTask(std::coroutine_handle<TPromiseType<T>>::from_promise(*this));
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

    T GetTaskResult() noexcept {
        return *TaskResult_;
    }

private:
    std::shared_ptr<T> TaskResult_ = nullptr;
    std::coroutine_handle<> Continuation_;
    std::exception_ptr Exception_;
};

template<typename T>
requires std::movable<T>
class TExecutorPromiseType : public TPromiseType<T> {
public:
    void Resume() {
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
        return TAsyncTask(std::coroutine_handle<TExecutorPromiseType>::from_promise(*this));
    }

    void SetExecutor(IExecutorPtr executor) const noexcept {
        Executor_ = std::move(executor);
    }

private:
    mutable IExecutorPtr Executor_ = nullptr;
};

template<std::movable T, std::derived_from<TPromiseType<T>> P = TPromiseType<T>>
class TAsyncTask {
public:
    using THandle = std::coroutine_handle<P>;

    TAsyncTask(THandle h)
        :Handle_(h) {}

    TAsyncTask(const TAsyncTask&) = delete;
    TAsyncTask& operator=(const TAsyncTask&) = delete;
    TAsyncTask(TAsyncTask&&) = default;
    TAsyncTask& operator=(TAsyncTask&&) = default;

    const P GetPromise() const {
        return Handle_.promise();
    }

    auto& Run() {
        Handle_.promise().Resume();
        return *this;
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
            TaskHandle.promise().SetContinuation(std::move(handle));
            return TaskHandle;
        }

        T await_resume() {
            if (TaskHandle.promise().Exception_) std::rethrow_exception(TaskHandle.promise().Exception_);
            return *TaskHandle.promise().GetTaskResult();
        }
    };

    TTaskAwaiter operator co_await() const noexcept {
        return TTaskAwaiter(Handle_);
    }

private:
    THandle Handle_ = nullptr;
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
