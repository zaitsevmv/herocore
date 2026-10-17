#pragma once

#include <concepts>
#include <coroutine>
#include <exception>
#include <utility>

namespace NAsync {

class IResumable {
public:
    virtual void Resume() = 0;
protected:
    ~IResumable() = default;
};

class TIntrusiveAwaiter {
public:
    void Continue() {
        if (Handle_ && !Handle_.done()) Promise_->Resume();
    }

protected:
    template<std::derived_from<IResumable> P>
    void SetHandle(std::coroutine_handle<P> handle) {
        Handle_ = handle;
        Promise_ = &handle.promise();
    }

private:
    std::coroutine_handle<> Handle_;
    IResumable* Promise_;
};

template<typename THandle, typename T>
class TTaskAwaiter {
public:
    TTaskAwaiter(THandle handle)
        : TaskHandle_(std::move(handle)) {}

    operator bool() const {
        return TaskHandle_ != nullptr;
    }

    bool await_ready() const {
        return TaskHandle_.done();
    }

    auto await_suspend(std::coroutine_handle<> handle) noexcept {
        TaskHandle_.promise().SetContinuation(std::move(handle));
        return TaskHandle_;
    }

    T await_resume() {
        if (TaskHandle_.promise().GetException()) std::rethrow_exception(TaskHandle_.promise().GetException());
        return TaskHandle_.promise().GetTaskResult();
    }
private:
    THandle TaskHandle_ = nullptr;
};

} // namespace NUtils
