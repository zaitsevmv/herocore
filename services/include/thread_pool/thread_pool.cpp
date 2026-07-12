#include "thread_pool.h"

#include <functional>
#include <stop_token>

namespace NAsync {

void TThreadPool::QueueWaitOperation() {
    auto stoken = StopSource_.get_token();
    while (!stoken.stop_requested() || !OperationsQueue_.Empty()) {
        QueueSemaphore_.acquire();
        if (auto op = OperationsQueue_.TryPop(); op) {
            try {
                op.value()();
            } catch (...) {}
        }
    }
}

void TThreadPool::AssingWorkers() {
    Threads_.clear();
    for (size_t i = 0u; i < ThreadCount_; i++) {
        Threads_.emplace_back([this](){return QueueWaitOperation();});
    }
}

TThreadPool::TThreadPool(size_t threadCount)
    : ThreadCount_{threadCount}, StopSource_(), QueueSemaphore_(0)
{
    AssingWorkers();
}

TThreadPool::~TThreadPool() {
    Stop();
}

void TThreadPool::Stop() {
    StopSource_.request_stop();
    OperationsQueue_.Clear();
    QueueSemaphore_.release(ThreadCount_);
    for (auto& th: Threads_) {
        th.join();
    }
}

void TThreadPool::Wait() {
    StopSource_.request_stop();
    QueueSemaphore_.release(ThreadCount_);
    for (auto& th: Threads_) {
        th.join();
    }
    std::stop_source newSource;
    StopSource_.swap(newSource);
    AssingWorkers();
}

void TThreadPool::Append(std::function<void()> op) {
    OperationsQueue_.Push(std::move(op));
    QueueSemaphore_.release();
}

} // namespace NAsync
