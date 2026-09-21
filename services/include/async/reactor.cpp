#include "reactor.h"

#include <atomic>
#include <cerrno>
#include <cstring>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <stdexcept>

#include <liburing.h>

#include <include/executor/thread_pool.h>

using namespace NAsync;

TReactor::TReactor()
    : Executor_(std::make_shared<TThreadPool>(1)) {
    memset(&RingParams_, 0, sizeof(RingParams_));
    auto ret = io_uring_queue_init_params(256, &Ring_, &RingParams_);
    if (ret != 0) {
        throw std::runtime_error("reactor: error creating uring");
    }
}

TReactor::TReactor(IExecutorPtr executor)
    : Executor_(std::move(executor)) {
    memset(&RingParams_, 0, sizeof(RingParams_));
    auto ret = io_uring_queue_init_params(4096, &Ring_, &RingParams_);
    if (ret != 0) {
        throw std::runtime_error("reactor: error creating uring");
    }
}

void TReactor::Run(std::stop_token stoken) {
    while (!stoken.stop_requested() || PendingOps_.load(std::memory_order::relaxed) > 0) {
        RunOnce();
    }
}

bool TReactor::RegisterHandle(void* userData, int fd, EOperation opType, TReactorCtx ctx) {
    std::lock_guard<std::mutex> lock(UringMutex_);
    io_uring_sqe* sqe = io_uring_get_sqe(&Ring_);
    if (sqe) {
        switch (opType) {
            case EOperation::Read: {
                auto readSpan = std::get<std::span<char>>(ctx.Data);
                io_uring_prep_recv(sqe, fd, readSpan.data(), readSpan.size(), 0);
                break;
            };
            case EOperation::ReadFile: {
                auto readSpan = std::get<std::span<char>>(ctx.Data);
                io_uring_prep_read(sqe, fd, readSpan.data(), readSpan.size(), ctx.Offset);
                break;
            };
            case EOperation::Write: {
                auto writeSpan = std::get<std::span<const char>>(ctx.Data);
                io_uring_prep_write(sqe, fd, writeSpan.data(), writeSpan.size(), ctx.Offset);
                break;
            };
            case EOperation::Accept: {
                io_uring_prep_accept(sqe, fd, nullptr, nullptr, 0);
                break;
            };
            case EOperation::Connect: {
                io_uring_prep_connect(sqe, fd, ctx.Addr, ctx.AddrLen);
                break;
            }
        }
        io_uring_sqe_set_data(sqe, userData);
        PendingOps_.fetch_add(1, std::memory_order::relaxed);
        return true;
    } else {
        reinterpret_cast<TReactorAwaiter*>(userData)->SetResult(std::nullopt);
    }
    return false;
}

void TReactor::RunOnce() {
    std::lock_guard<std::mutex> lock(UringMutex_);
    int submitted = io_uring_submit(&Ring_);
    if (submitted < 0 && submitted != -EAGAIN) {
        throw std::system_error(-submitted, std::system_category(), "reactor: submit to sqe failed");
    }

    io_uring_cqe* cqe;
    unsigned head;
    unsigned count = 0;

    io_uring_peek_cqe(&Ring_, &cqe);
    
    io_uring_for_each_cqe(&Ring_, head, cqe) {
        count++;
        if (cqe->res == -ECANCELED) continue;
        if (cqe->user_data != 0) {
            auto* userData = reinterpret_cast<TReactorAwaiter*>(cqe->user_data);
            userData->SetResult(cqe->res);
            Executor_->Append(std::move(*userData));
        }
    }

    PendingOps_.fetch_sub(count);
    
    if (count > 0) {
        io_uring_cq_advance(&Ring_, count);
    }
}

void TReactorAwaiter::SetResult(std::optional<int> res) noexcept {
    Result_ = std::move(res);
}
