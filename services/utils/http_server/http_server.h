#pragma once

#include <functional>
#include <memory>

#include <include/async/reactor.h>
#include <include/async/async_task.h>
#include <include/thread_pool/thread_pool.h>

#include "http_message.h"

namespace NHttp {

class THttpServer {
private:
    class TImpl;

public:
    THttpServer();
    explicit THttpServer(NAsync::TReactorPtr reactor);

    void Listen(NAsync::TThreadPoolPtr threadPool, const size_t threadCount);
    void AddCallback(const std::string& target, std::function<void()> callback);
    void StopListen();

    NAsync::TAsyncTask<THttpResponse> SendRequest(const THttpRequest& request);
    NAsync::TAsyncTask<bool> SendResponse(const THttpResponse& response);

private:
    std::unique_ptr<TImpl> Pimpl_;
};

} // namespace NHttp
