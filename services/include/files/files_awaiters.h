#pragma once

#include <coroutine>
#include <cstddef>
#include <fcntl.h>
#include <span>

#include <include/async/reactor.h>

static constexpr size_t FILES_BUFFER_SIZE = 1024 * 1024 * 1024;

namespace NAsync {

class TFilesAwaiterBase {
public:
    TFilesAwaiterBase(TReactorPtr reactor, int fd, uint64_t offset);

protected:
    TReactorPtr Reactor_;
    TReactor::TUserDataPtr UserData_;
    int FileDesc_;
    uint64_t Offset_;
};

class TFilesReadAwaiter : public TFilesAwaiterBase {
public:
    TFilesReadAwaiter(TReactorPtr reactor, int fd, std::span<char> buffer, uint64_t offset);

    bool await_ready() const;

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> handle);

    size_t await_resume();

private:
    std::span<char> Data_;
};

class TFilesWriteAwaiter : public TFilesAwaiterBase {
public:
    TFilesWriteAwaiter(TReactorPtr reactor, int fd, std::span<const char> data, uint64_t offset);

    bool await_ready() const;

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> handle);

    size_t await_resume();

private:
    std::span<const char> Data_;
};

} // namespace NAsync
