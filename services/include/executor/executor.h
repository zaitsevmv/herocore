#pragma once

#include <functional>
#include <memory>

#include <include/async/base_awaiters.h>

namespace NAsync {

template<class... Ts>
struct overloads : Ts... { using Ts::operator()...; };

const auto ExecutorVisitor = overloads
{
    [](const std::function<void()>& op){ op(); },
    [](NAsync::TIntrusiveAwaiter& resumable){ resumable.Continue(); }
};

class IExecutor {
public:
    virtual ~IExecutor() = default;

    virtual void Append(std::function<void()> op) = 0;
    virtual void Append(TIntrusiveAwaiter&& resumable) = 0;
};

using IExecutorPtr = std::shared_ptr<IExecutor>;

} // namespace NAsync
