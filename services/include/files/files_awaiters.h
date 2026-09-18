#pragma once

#include <coroutine>
#include <cstddef>
#include <fcntl.h>
#include <span>

#include <include/async/reactor.h>

static constexpr size_t FILES_BUFFER_SIZE = 1024 * 1024 * 1024;

namespace NAsync {

class TFilesAwaiterBase : public TReactorAwaiter {
public:
    TFilesAwaiterBase(TReactorPtr reactor, int fd, uint64_t offset);

protected:
    int FileDesc_;
    uint64_t Offset_;
};

class TFilesReadAwaiter : public TFilesAwaiterBase {
public:
    TFilesReadAwaiter(TReactorPtr reactor, int fd, std::span<char> buffer, uint64_t offset);

    bool await_ready() const;

    template<typename P>
    std::coroutine_handle<> await_suspend(std::coroutine_handle<P> handle) {
        SetHandle(handle);
        if (!Reactor_->RegisterHandle(this, FileDesc_, TReactor::EOperation::Read,
            TReactorCtx{
                .Data = Data_
            }
        )) {
            Result_ = std::nullopt;
            return handle;
        }
        return std::noop_coroutine();
    }

    size_t await_resume();

private:
    std::span<char> Data_;
};

class TFilesWriteAwaiter : public TFilesAwaiterBase {
public:
    TFilesWriteAwaiter(TReactorPtr reactor, int fd, std::span<const char> data, uint64_t offset);

    bool await_ready() const;

    template<typename P>
    std::coroutine_handle<> await_suspend(std::coroutine_handle<P> handle) {
        SetHandle(handle);
        if (!Reactor_->RegisterHandle(this, FileDesc_, TReactor::EOperation::Write, 
            TReactorCtx{
                .Data = std::span<char>(const_cast<char*>(Data_.data()), Data_.size())
            }
        )) {
            Result_ = std::nullopt;
            return handle;
        }
        return std::noop_coroutine();
    }

    size_t await_resume();

private:
    std::span<const char> Data_;
};

} // namespace NAsync
