#include "files_awaiters.h"

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

using namespace NAsync;

TFilesAwaiterBase::TFilesAwaiterBase(TReactorPtr reactor, int fd, uint64_t offset)
    : Reactor_(reactor), FileDesc_(fd), UserData_(std::make_shared<TReactor::TUserData>()), Offset_(offset) {}


TFilesReadAwaiter::TFilesReadAwaiter(TReactorPtr reactor, int fd, std::span<char> buffer, uint64_t offset)
    : TFilesAwaiterBase(reactor, fd, offset), Data_(buffer) {}

bool TFilesReadAwaiter::await_ready() const {
    return false;
}

std::coroutine_handle<> TFilesReadAwaiter::await_suspend(std::coroutine_handle<> handle) {
    UserData_->Handle = handle;
    if (!Reactor_->RegisterHandle(UserData_, FileDesc_, TReactor::EOperation::Read,
        TReactorCtx{
            .Data = Data_
        }
    )) {
        UserData_->Cqe = nullptr;
        return handle;
    }
    return std::noop_coroutine();
}

size_t TFilesReadAwaiter::await_resume() {
    if (!UserData_->Cqe) {
        throw std::runtime_error("TCP Read: Failed to submit request to reactor");
    }
    auto result = UserData_->Cqe->res;
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

std::coroutine_handle<> TFilesWriteAwaiter::await_suspend(std::coroutine_handle<> handle) {
    UserData_->Handle = handle;
    if (!Reactor_->RegisterHandle(UserData_, FileDesc_, TReactor::EOperation::Write, 
        TReactorCtx{
            .Data = std::span<char>(const_cast<char*>(Data_.data()), Data_.size())
        }
    )) {
        UserData_->Cqe = nullptr;
        return handle;
    }
    return std::noop_coroutine();
}

size_t TFilesWriteAwaiter::await_resume() {
    if (!UserData_->Cqe) {
        throw std::runtime_error("TCP Write: Failed to submit request to reactor");
    }
    auto result = UserData_->Cqe->res;
    if (result < 0) {
        throw std::system_error(-result, std::system_category(), "TCP Write failed");
    }
    return static_cast<size_t>(result);
}
