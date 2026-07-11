#include "reactor.h"
#include <liburing.h>

#include <atomic>
#include <cstring>
#include <mutex>
#include <shared_mutex>
#include <span>
#include <stdexcept>

using namespace NAsync;

TReactor::TReactor() {
    memset(&RingParams_, 0, sizeof(RingParams_));
    auto ret = io_uring_queue_init_params(4, &Ring_, &RingParams_);
    if (ret != 0) {
        throw std::runtime_error("reactor: error creating uring");
    }
}

void TReactor::Run(std::stop_token stoken) {
    while (!stoken.stop_requested() || PendingOps_.load(std::memory_order::relaxed) > 0) {
        RunOnce();
    }
}

bool TReactor::RegisterHandle(TUserDataPtr userData, int fd, EOperation opType, TReactorCtx ctx) {
    std::lock_guard<std::mutex> lock(UringMutex_);
    io_uring_sqe* sqe = io_uring_get_sqe(&Ring_);
    if (sqe) {
        switch (opType) {
            case EOperation::Read: {
                io_uring_prep_recv(sqe, fd, ctx.Data.data(), ctx.Data.size(), 0);
                break;
            };
            case EOperation::ReadFile: {
                io_uring_prep_read(sqe, fd, ctx.Data.data(), ctx.Data.size(), ctx.Offset);
                break;
            };
            case EOperation::Write: {
                io_uring_prep_write(sqe, fd, ctx.Data.data(), ctx.Data.size(), ctx.Offset);
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
        io_uring_sqe_set_data(sqe, userData.get());
        PendingOps_.fetch_add(1, std::memory_order::relaxed);
        return true;
    } else {
        userData->Cqe = nullptr;
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
            TReactor::TUserData* userData = reinterpret_cast<TUserData*>(cqe->user_data);
            userData->Cqe = cqe;
            userData->Handle.resume();
        }
    }

    PendingOps_.fetch_sub(count);
    
    if (count > 0) {
        io_uring_cq_advance(&Ring_, count);
    }
}
