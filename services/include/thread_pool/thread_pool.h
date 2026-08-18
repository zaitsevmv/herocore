#pragma once

// things I dont like here
// - wait free queue
// - stop and wait seem strange

#include <include/thread_safe/queue.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <semaphore>
#include <stop_token>
#include <thread>

namespace NAsync {

class TThreadPool {
public:
    TThreadPool(size_t threadCount);
    ~TThreadPool();

    void Append(std::function<void()> op);
    void Stop();
    void Wait();

private:
    void QueueWaitOperation();
    void AssingWorkers();

    size_t ThreadCount_ = 0;
    std::stop_source StopSource_;
    std::vector<std::jthread> Threads_;

    std::counting_semaphore<> QueueSemaphore_;
    TQueueSafe<std::function<void()>> OperationsQueue_;
};

using TThreadPoolPtr = std::shared_ptr<TThreadPool>;

} // namespace NAsync
