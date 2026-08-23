#include "tcp_awaiters.h"

#include <sys/socket.h>

#include <coroutine>
#include <cstddef>
#include <cstring>
#include <memory>
#include <span>
#include <stdexcept>
#include <system_error>

#include <liburing.h>

#include <include/async/reactor.h>
#include <include/network/fd_utils.h>

using namespace NAsync;

TTCPAwaiterBase::TTCPAwaiterBase(TReactorPtr reactor, int socketDesc)
    : Reactor_(reactor), Socket_(socketDesc), UserData_(std::make_shared<TReactor::TUserData>()) {}


TTCPReadAwaiter::TTCPReadAwaiter(TReactorPtr reactor, int socketDesc, std::span<char> buffer)
    :TTCPAwaiterBase(reactor, socketDesc), Data_(buffer) {}

bool TTCPReadAwaiter::await_ready() const {
    return false;
}

std::coroutine_handle<> TTCPReadAwaiter::await_suspend(std::coroutine_handle<> handle) {
    UserData_->Handle = handle;
    if (!Reactor_->RegisterHandle(UserData_, Socket_, TReactor::EOperation::Read,
        TReactorCtx{
            .Data = Data_
        }
    )) {
        UserData_->Cqe = nullptr;
        return handle;
    }
    return std::noop_coroutine();
}

size_t TTCPReadAwaiter::await_resume() {
    if (!UserData_->Cqe) {
        throw std::runtime_error("TCP Read: Failed to submit request to reactor");
    }
    auto result = UserData_->Cqe->res;
    if (result < 0) {
        throw std::system_error(-result, std::system_category(), "TCP Read failed");
    }
    return result;
}


TTCPWriteAwaiter::TTCPWriteAwaiter(TReactorPtr reactor, int socketDesc, std::span<const char> data)
    :TTCPAwaiterBase(reactor, socketDesc), Data_(data) {}

bool TTCPWriteAwaiter::await_ready() const {
    return false;
}

std::coroutine_handle<> TTCPWriteAwaiter::await_suspend(std::coroutine_handle<> handle) {
    UserData_->Handle = handle;
    if (!Reactor_->RegisterHandle(UserData_, Socket_, TReactor::EOperation::Write, 
        TReactorCtx{
            .Data = Data_
        }
    )) {
        UserData_->Cqe = nullptr;
        return handle;
    }
    return std::noop_coroutine();
}

size_t TTCPWriteAwaiter::await_resume() {
    if (!UserData_->Cqe) {
        throw std::runtime_error("TCP Write: Failed to submit request to reactor");
    }
    auto result = UserData_->Cqe->res;
    if (result < 0) {
        throw std::system_error(-result, std::system_category(), "TCP Write failed");
    }
    return static_cast<size_t>(result);
}


TTCPAcceptAwaiter::TTCPAcceptAwaiter(TReactorPtr reactor, int socketDesc)
    : TTCPAwaiterBase(reactor, socketDesc) {}

bool TTCPAcceptAwaiter::await_ready() const {
    return UserData_->Cqe != nullptr;
}

std::coroutine_handle<> TTCPAcceptAwaiter::await_suspend(std::coroutine_handle<> handle) {
    UserData_->Handle = handle;
    if (!Reactor_->RegisterHandle(UserData_, Socket_, TReactor::EOperation::Accept, {})) {
        UserData_->Cqe = nullptr;
        return handle;
    }
    return std::noop_coroutine();
}

int TTCPAcceptAwaiter::await_resume() {
    if (!UserData_->Cqe) {
        throw std::runtime_error("TCP Accept: Failed to submit request to reactor");
    }
    int clientSocket = UserData_->Cqe->res;
    if (clientSocket < 0) {
        throw std::system_error(-clientSocket, std::system_category(), "TCP Accept failed");
    }
    NUtils::SetSocketNonblocking(clientSocket);
    return clientSocket;
}


TTCPConnectAwaiter::TTCPConnectAwaiter(TReactorPtr reactor, int socketDesc, const sockaddr* addr, socklen_t addrLen)
    : TTCPAwaiterBase(reactor, socketDesc), AddrLen_(addrLen) {
    std::memcpy(&AddrStorage_, addr, addrLen);
}

bool TTCPConnectAwaiter::await_ready() const {
    return false;
}

std::coroutine_handle<> TTCPConnectAwaiter::await_suspend(std::coroutine_handle<> handle) {
    UserData_->Handle = handle;
    if (!Reactor_->RegisterHandle(UserData_, Socket_, TReactor::EOperation::Connect, 
        TReactorCtx{
            .Addr = reinterpret_cast<sockaddr*>(&AddrStorage_),
            .AddrLen = AddrLen_
        }
    )) {
        UserData_->Cqe = nullptr;
        return handle;
    }
    return std::noop_coroutine();
}

void TTCPConnectAwaiter::await_resume() {
    if (!UserData_->Cqe) {
        throw std::runtime_error("TCP Connect: Failed to submit request to reactor");
    }
    auto res = UserData_->Cqe->res;
    if (res < 0) {
        throw std::system_error(-res, std::system_category(), "TCP Connect failed");
    }
}
