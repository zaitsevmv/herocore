#pragma once

#include <functional>
#include <memory>

#include <include/async/reactor.h>
#include <include/async/async_task.h>
#include <include/thread_pool/thread_pool.h>

#include "http_message.h"

namespace NHttp {

struct THttpCallback {
    std::string Target;
    EHttpMethod Method;
    std::function<void()> Handler;
};

class THttpServer {
private:
    class TImpl;

public:
    explicit THttpServer(NAsync::TReactorPtr reactor);
    ~THttpServer();

    void Listen(NAsync::TThreadPoolPtr threadPool, const size_t threadCount, const uint16_t port);
    void AddCallback(const THttpCallback& callback);
    void StopListen();

    NAsync::TAsyncTask<THttpResponsePtr> SendRequest(const THttpRequestPtr request, std::string host, const uint16_t port);
    NAsync::TAsyncTask<bool> SendResponse(const THttpResponsePtr response, int clientFd);

private:
    std::unique_ptr<TImpl> Pimpl_;
};

} // namespace NHttp
