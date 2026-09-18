#include "files_awaiters.h"

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

using namespace NAsync;

TFilesAwaiterBase::TFilesAwaiterBase(TReactorPtr reactor, int fd, uint64_t offset)
    : FileDesc_(fd), Offset_(offset) {
    Reactor_ = std::move(reactor);
}


TFilesReadAwaiter::TFilesReadAwaiter(TReactorPtr reactor, int fd, std::span<char> buffer, uint64_t offset)
    : TFilesAwaiterBase(reactor, fd, offset), Data_(buffer) {}

bool TFilesReadAwaiter::await_ready() const {
    return false;
}

size_t TFilesReadAwaiter::await_resume() {
    if (!Result_) {
        throw std::runtime_error("TCP Read: Failed to submit request to reactor");
    }
    auto result = *Result_;
    if (result < 0) {
        throw std::system_error(-result, std::system_category(), "TCP Read failed");
    }
    return result;
}


TFilesWriteAwaiter::TFilesWriteAwaiter(TReactorPtr reactor, int fd, std::span<const char> data, uint64_t offset)
    : TFilesAwaiterBase(reactor, fd, offset), Data_(data) {}

bool TFilesWriteAwaiter::await_ready() const {
    return false;
}

size_t TFilesWriteAwaiter::await_resume() {
    if (!Result_) {
        throw std::runtime_error("TCP Write: Failed to submit request to reactor");
    }
    const auto result = *Result_;
    if (result < 0) {
        throw std::system_error(-result, std::system_category(), "TCP Write failed");
    }
    return static_cast<size_t>(result);
}
