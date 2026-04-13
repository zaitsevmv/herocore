#pragma once

#include <liburing.h>
#include <coroutine>
#include <liburing/io_uring.h>
#include <memory>
#include <span>
#include <stop_token>

namespace NAsync {

struct TReactorCtx {
    std::span<char> Data = std::span<char>();
    sockaddr* Addr = nullptr;
    size_t AddrLen = 0;
};

class TReactor {
public:
    struct TUserData {
        io_uring_cqe* Cqe;
        std::coroutine_handle<> Handle;
    };
    using TUserDataPtr = std::shared_ptr<TUserData>;

    enum class EOperation {
        Read, Write, Accept, Connect
    };

    TReactor();

    TReactor(const TReactor&) = delete;
    TReactor& operator=(const TReactor&) = delete;
    TReactor(TReactor&&) = default;
    TReactor& operator=(TReactor&&) = default;

    void Run(std::stop_token stoken);

    bool RegisterHandle(TUserDataPtr userData, int fd, EOperation opType, TReactorCtx ctx);

private:
    void RunOnce();

private:
    io_uring Ring_;
    io_uring_params RingParams_;
};

using TReactorPtr = std::shared_ptr<TReactor>;

} // namespace NAsync
