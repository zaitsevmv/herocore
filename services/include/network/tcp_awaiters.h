#pragma once

#include "async/reactor.h"

#include <coroutine>
#include <cstddef>
#include <fcntl.h>
#include <span>

static constexpr size_t TCP_BUFFER_SIZE = 10 * 1024 * 1024;

namespace NAsync {

class TTCPAwaiterBase {
public:
    TTCPAwaiterBase(TReactorPtr reactor, int socketDesc);

protected:
    TReactorPtr Reactor_;
    TReactor::TUserDataPtr UserData_;
    int Socket_;
};

class TTCPReadAwaiter : public TTCPAwaiterBase {
public:
    TTCPReadAwaiter(TReactorPtr reactor, int socketDesc, std::span<char> buffer);

    bool await_ready() const;

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> handle);

    size_t await_resume();

private:
    std::span<char> Data_;
};

class TTCPWriteAwaiter : public TTCPAwaiterBase {
public:
    TTCPWriteAwaiter(TReactorPtr reactor, int socketDesc, std::span<char> data);

    bool await_ready() const;

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> handle);

    size_t await_resume();

private:
    std::span<char> Data_;
};

class TTCPAcceptAwaiter : public TTCPAwaiterBase {
public:
    TTCPAcceptAwaiter(TReactorPtr reactor, int socketDesc);

    bool await_ready() const;

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> handle);

    int await_resume();
};

class TTCPConnectAwaiter : public TTCPAwaiterBase {
public:
    TTCPConnectAwaiter(TReactorPtr reactor, int socketDesc, const sockaddr* addr, socklen_t addrLen);

    bool await_ready() const;

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> handle);

    void await_resume();

private:
    sockaddr_storage AddrStorage_;
    socklen_t AddrLen_;
};

} // namespace NAsync
