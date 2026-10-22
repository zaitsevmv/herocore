#pragma once

#include <memory>

#include <lib/base/async/reactor.h>
#include <lib/base/async/async_task.h>
#include <lib/base/executor/thread_pool.h>
#include <lib/utils/rpc_server/proto/rpc.pb.h>

#include "rpc_message.h"

namespace NRpc {

class TRpcConnection;

class TRpcServer {
private:
    class TImpl;

public:
    explicit TRpcServer(NAsync::TReactorPtr reactor);
    ~TRpcServer();

    void Listen(NAsync::TThreadPoolPtr threadPool, const size_t threadCount, const uint16_t port);
    void StopListen();

    using THandleType = std::function<NAsync::TAsyncTask<IRpcMessagePtr>()>;
    void AddCallback(const std::string_view messageTypeName, const THandleType handle);

    NAsync::TAsyncTask<IRpcMessagePtr> SendMessage(const IRpcMessagePtr message, TRpcConnection client);

private:
    std::unique_ptr<TImpl> Pimpl_;
};

} // namespace NRpc
