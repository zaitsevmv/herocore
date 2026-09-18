#pragma once

#include <fcntl.h>

#include <coroutine>
#include <cstddef>
#include <span>

#include <include/async/reactor.h>
#include <liburing/io_uring.h>

static constexpr size_t TCP_BUFFER_SIZE = 10 * 1024 * 1024;

namespace NAsync {

class TTCPAwaiterBase : public TReactorAwaiter {
public:
    TTCPAwaiterBase(TReactorPtr reactor, int socketDesc);

protected:
    int Socket_;
};

class TTCPReadAwaiter : public TTCPAwaiterBase {
public:
    TTCPReadAwaiter(TReactorPtr reactor, int socketDesc, std::span<char> buffer);

    bool await_ready() const;

    template<typename P>
    std::coroutine_handle<> await_suspend(std::coroutine_handle<P> handle) {
        SetHandle(handle);
        if (!Reactor_->RegisterHandle(this, Socket_, TReactor::EOperation::Read,
            TReactorCtx{
                .Data = Data_
            }
        )) {
            Result_ = -1;
            return handle;
        }
        return std::noop_coroutine();
    }

    size_t await_resume();

private:
    std::span<char> Data_;
};

class TTCPWriteAwaiter : public TTCPAwaiterBase {
public:
    TTCPWriteAwaiter(TReactorPtr reactor, int socketDesc, std::span<const char> data);

    bool await_ready() const;

    template<typename P>
    std::coroutine_handle<> await_suspend(std::coroutine_handle<P> handle) {
        SetHandle(handle);
        if (!Reactor_->RegisterHandle(this, Socket_, TReactor::EOperation::Write, 
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
    std::span<const char> Data_;
};

class TTCPAcceptAwaiter : public TTCPAwaiterBase {
public:
    TTCPAcceptAwaiter(TReactorPtr reactor, int socketDesc);

    bool await_ready() const;

    template<typename P>
    std::coroutine_handle<> await_suspend(std::coroutine_handle<P> handle) {
        SetHandle(handle);
        if (!Reactor_->RegisterHandle(this, Socket_, TReactor::EOperation::Accept, {})) {
            Result_ = std::nullopt;
            return handle;
        }
        return std::noop_coroutine();
    }

    int await_resume();
};

class TTCPConnectAwaiter : public TTCPAwaiterBase {
public:
    TTCPConnectAwaiter(TReactorPtr reactor, int socketDesc, const sockaddr* addr, socklen_t addrLen);

    bool await_ready() const;

    template<typename P>
    std::coroutine_handle<> await_suspend(std::coroutine_handle<P> handle) {
        SetHandle(handle);
        if (!Reactor_->RegisterHandle(this, Socket_, TReactor::EOperation::Connect, 
            TReactorCtx{
                .Addr = reinterpret_cast<sockaddr*>(&AddrStorage_),
                .AddrLen = AddrLen_
            }
        )) {
            Result_ = std::nullopt;
            return handle;
        }
        return std::noop_coroutine();
    }

    void await_resume();

private:
    sockaddr_storage AddrStorage_;
    socklen_t AddrLen_;
};

} // namespace NAsync
