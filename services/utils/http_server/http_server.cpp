#include "http_server.h"

#include <exception>
#include <functional>
#include <iostream>
#include <stop_token>
#include <unordered_map>

#include <include/network/tcp_requests.h>
#include "include/network/fd_utils.h"

namespace {

} // namespace

namespace NHttp {

class THttpServer::TImpl {
public:
    explicit TImpl(NAsync::TReactorPtr reactor);
    ~TImpl();

    void Listen(NAsync::TThreadPoolPtr threadPool, const size_t threadCount);
    void AddCallback(const std::string& target, std::function<void()> callback);
    void StopListen();

    NAsync::TAsyncTask<THttpResponse> SendRequest(const THttpRequest& request);
    NAsync::TAsyncTask<bool> SendResponse(const THttpResponse& response);

private:
    NAsync::TAsyncTask<bool> HandleRequest();

    NAsync::TReactorPtr Reactor_;
    std::unordered_map<std::string, std::function<void()>> Callbacks_;
    std::stop_source StopSource_;
};


THttpServer::TImpl::TImpl(NAsync::TReactorPtr reactor)
    : Reactor_(std::move(reactor)) {}

THttpServer::TImpl::~TImpl() = default;

void THttpServer::TImpl::Listen(NAsync::TThreadPoolPtr threadPool, const size_t threadCount) {
    for (size_t i = 0u; i < threadCount; i++) {
        threadPool->Append(
            [this, stoken = StopSource_.get_token()]() {
                HandleRequest();
            }
        );
    }
}

void THttpServer::TImpl::StopListen() {
    StopSource_.request_stop();
}

void THttpServer::TImpl::AddCallback(const std::string& target, std::function<void()> callback) {
    Callbacks_.emplace(target, std::move(callback));
}

NAsync::TAsyncTask<bool> THttpServer::TImpl::HandleRequest() {
    int serverFd = NUtils::CreateListeningSocket(22);
    try {
        auto client = co_await NUtils::AcceptConnectionAsync(Reactor_, serverFd);
        bool msgComplete = false;
        THttpRequest clientRequest;
        while (!msgComplete) {
            std::string msg = co_await NUtils::ReadAsync(Reactor_, client);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling request: " << e.what() << std::endl;
    }
    co_return co_await HandleRequest();
}

} // namespace NHttp
