#pragma once

#include <coroutine>
#include <memory>
#include <span>
#include <stop_token>
#include <variant>

#include <liburing.h>
#include <liburing/io_uring.h>

#include <include/thread_pool/thread_pool.h>

namespace NAsync {

struct TReactorCtx {
    std::variant<std::span<char>, std::span<const char>> Data = std::span<char>();
    sockaddr* Addr = nullptr;
    size_t AddrLen = 0;
    uint64_t Offset = 0ull;
};

class TReactor {
public:
    struct TUserData {
        io_uring_cqe* Cqe;
        std::coroutine_handle<> Handle;
    };
    using TUserDataPtr = std::shared_ptr<TUserData>;

    enum class EOperation: short {
        Read, ReadFile, Write, Accept, Connect
    };

    TReactor();
    explicit TReactor(TThreadPoolPtr threadPool);

    TReactor(const TReactor&) = delete;
    TReactor& operator=(const TReactor&) = delete;
    TReactor(TReactor&&) = delete;
    TReactor& operator=(TReactor&&) = delete;

    void Run(std::stop_token stoken);

    bool RegisterHandle(TUserDataPtr userData, int fd, EOperation opType, TReactorCtx ctx);

private:
    void RunOnce();

private:
    io_uring Ring_;
    io_uring_params RingParams_;

    std::atomic<ssize_t> PendingOps_ = 0u;
    std::mutex UringMutex_;

    TThreadPoolPtr ThreadPool_;
};

using TReactorPtr = std::shared_ptr<TReactor>;

} // namespace NAsync
