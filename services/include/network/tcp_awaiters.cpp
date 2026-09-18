#include "tcp_awaiters.h"

#include <sys/socket.h>

#include <coroutine>
#include <cstddef>
#include <cstring>
#include <optional>
#include <span>
#include <stdexcept>
#include <system_error>

#include <liburing.h>

#include <include/async/reactor.h>
#include <include/network/fd_utils.h>

using namespace NAsync;

TTCPAwaiterBase::TTCPAwaiterBase(TReactorPtr reactor, int socketDesc)
    : Socket_(socketDesc) {
    Reactor_ = std::move(reactor);
}


TTCPReadAwaiter::TTCPReadAwaiter(TReactorPtr reactor, int socketDesc, std::span<char> buffer)
    :TTCPAwaiterBase(reactor, socketDesc), Data_(buffer) {}

bool TTCPReadAwaiter::await_ready() const {
    return false;
}

size_t TTCPReadAwaiter::await_resume() {
    if (!Result_) {
        throw std::runtime_error("TCP Read: Failed to submit request to reactor");
    }
    const auto result = *Result_;
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

size_t TTCPWriteAwaiter::await_resume() {
    if (!Result_) {
        throw std::runtime_error("TCP Write: Failed to submit request to reactor");
    }
    const auto result = *Result_;
    if (result < 0) {
        throw std::system_error(-result, std::system_category(), "TCP Write failed");
    }
    return static_cast<size_t>(result);
}


TTCPAcceptAwaiter::TTCPAcceptAwaiter(TReactorPtr reactor, int socketDesc)
    : TTCPAwaiterBase(reactor, socketDesc) {}

bool TTCPAcceptAwaiter::await_ready() const {
    return Result_ != std::nullopt;
}

int TTCPAcceptAwaiter::await_resume() {
    if (!Result_) {
        throw std::runtime_error("TCP Accept: Failed to submit request to reactor");
    }
    int clientSocket = *Result_;
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

void TTCPConnectAwaiter::await_resume() {
    if (!Result_) {
        throw std::runtime_error("TCP Connect: Failed to submit request to reactor");
    }
    if (*Result_ < 0) {
        throw std::system_error(-*Result_, std::system_category(), "TCP Connect failed");
    }
}
