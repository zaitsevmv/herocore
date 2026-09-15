#pragma once

#include <functional>
#include <memory>

namespace NAsync {

class IExecutor {
public:
    virtual ~IExecutor() = default;

    virtual void Append(std::function<void()> op) = 0;
};

using IExecutorPtr = std::unique_ptr<IExecutor>;

} // namespace NAsync
