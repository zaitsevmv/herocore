#include "timed_runner.h"

#include "thread_pool/thread_pool.h"
#include "thread_safe/queue.h"

#include <atomic>
#include <compare>
#include <memory>
#include <mutex>
#include <queue>
#include <stop_token>

using namespace NAsync;

class TTimedRunner::TImpl {
public:
    explicit TImpl(TThreadPoolPtr threadPool);
    ~TImpl() = default;

    void AddToRunner(std::function<void()> task, const TTimePointType deadline);
    void RequestStop();
private:
    void Loop(std::stop_token stoken);
    void RegisterHandle(std::function<void()> task, const TTimePointType deadline);

    struct TTask {
        TTimePointType TimePoint;
        std::function<void()> Handle;

        constexpr std::strong_ordering operator<=>(const TTask& rhs) const {
            if (TimePoint < rhs.TimePoint) {
                return std::strong_ordering::less;
            } else if (TimePoint > rhs.TimePoint) {
                return std::strong_ordering::greater;
            }
            return std::strong_ordering::equal;
        }
    };

    TThreadPoolPtr ThreadPool_;
    std::mutex QueueMutex_;
    std::priority_queue<TTask, std::vector<TTask>, std::greater<>> TasksQueue_;
    std::stop_source StopSource_;

    std::atomic_size_t QueueSize_ = 0u;
};

void TTimedRunner::TImpl::Loop(std::stop_token stoken) {
    while (!stoken.stop_requested()) {
        std::lock_guard<std::mutex> lock(QueueMutex_);
        if (!TasksQueue_.empty() && TasksQueue_.top().TimePoint <= TTimePointType::clock::now()) {
            TasksQueue_.top().Handle();
            TasksQueue_.pop();
            --QueueSize_;
        }
    }
}

void TTimedRunner::TImpl::RequestStop() {
    while (QueueSize_ > 0u) {}
    StopSource_.request_stop();
}

void TTimedRunner::TImpl::RegisterHandle(std::function<void()> task, const TTimePointType deadline) {
    std::lock_guard<std::mutex> lock(QueueMutex_);
    TasksQueue_.push(TTask{
        .TimePoint = deadline,
        .Handle = std::move(task)
    });
    ++QueueSize_;
}

TTimedRunner::TImpl::TImpl(TThreadPoolPtr threadPool)
    : ThreadPool_(threadPool), StopSource_() {
    ThreadPool_->Append(
        [this]() {
            Loop(StopSource_.get_token());
        }
    );
}

void TTimedRunner::TImpl::AddToRunner(std::function<void()> task, const TTimePointType deadline) {
    RegisterHandle(std::move(task), deadline);
}

TTimedRunner::~TTimedRunner() = default;

TTimedRunner::TTimedRunner(TTimedRunner&&) = default;
TTimedRunner& TTimedRunner::operator=(TTimedRunner&&) = default;

TTimedRunner::TTimedRunner() {
    TThreadPoolPtr threadPool = std::make_shared<TThreadPool>(1);
    Pimpl_ = std::make_unique<TImpl>(threadPool);
}

TTimedRunner::TTimedRunner(TThreadPoolPtr threadPool)
    : Pimpl_(std::make_unique<TImpl>(threadPool)) {}

void TTimedRunner::AddToRunner(std::function<void()> task, const TDurationType duration) {
    return AddToRunner(std::move(task), TTimePointType::clock::now() + duration);
}

void TTimedRunner::AddToRunner(std::function<void()> task, const TTimePointType deadline) {
    return Pimpl_->AddToRunner(std::move(task), deadline);
}

void TTimedRunner::RequestStop() {
    return Pimpl_->RequestStop();
}
