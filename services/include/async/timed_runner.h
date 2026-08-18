#pragma once

#include <chrono>
#include <memory>

#include <include/thread_pool/thread_pool.h>

namespace NAsync {

using TDurationType = std::chrono::duration<long, std::ratio<1, 1000000000>>;
using TTimePointType = std::chrono::time_point<std::chrono::steady_clock, TDurationType>;

class TTimedRunner {
private:
    class TImpl;
public:
    TTimedRunner();
    explicit TTimedRunner(TThreadPoolPtr threadPool);

    ~TTimedRunner();

    TTimedRunner(TTimedRunner&&);
    TTimedRunner& operator=(TTimedRunner&&);
    TTimedRunner(TTimedRunner&) = delete;
    TTimedRunner& operator=(TTimedRunner&) = delete;

    void AddToRunner(std::function<void()> task, const TDurationType duration);
    void AddToRunner(std::function<void()> task, const TTimePointType deadline);

    void RequestStop();
private:
    std::unique_ptr<TImpl> Pimpl_;
};

using TTimedRunnerPtr = std::shared_ptr<TTimedRunner>;

} // namespace NAsync
