#include "http_server.h"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <stop_token>
#include <string>
#include <string_view>
#include <unordered_map>

#include <include/network/tcp_requests.h>
#include <include/async/async_task.h>
#include <include/network/fd_utils.h>
#include <utils/http_server/http_message.h>

namespace {

static constexpr std::string_view CRLF = "\r\n";
static constexpr std::string_view DCRLF = "\r\n\r\n";

static constexpr std::string_view CONTENT_LENGTH_HEADER = "Content-Length";
static constexpr size_t DEFAULT_BUFFER_SIZE = 1000;

} // namespace

namespace NHttp {

class THttpServer::TImpl {
public:
    explicit TImpl(NAsync::TReactorPtr reactor);
    ~TImpl();

    void Listen(NAsync::TThreadPoolPtr threadPool, const size_t threadCount, const uint16_t port);
    void AddCallback(const THttpCallback& callback);
    void StopListen();

    NAsync::TAsyncTask<THttpResponsePtr> SendRequest(const THttpRequestPtr request, std::string host, const uint16_t port);
    NAsync::TAsyncTask<bool> SendResponse(const THttpResponsePtr response, int clientFd);

    private:
    NAsync::TAsyncTask<bool> HandleRequests(int serverFd);
    NAsync::TAsyncTask<bool> ProcessRequest(const THttpRequestPtr request, int clientFd); 

    NAsync::TReactorPtr Reactor_;
    std::unordered_map<std::string_view, THttpCallback> Callbacks_;
    std::stop_source StopSource_;
};


THttpServer::TImpl::TImpl(NAsync::TReactorPtr reactor)
    : Reactor_(std::move(reactor)) {}

THttpServer::TImpl::~TImpl() = default;

void THttpServer::TImpl::Listen(NAsync::TThreadPoolPtr threadPool, const size_t threadCount, const uint16_t port) {
    int serverFd = NUtils::CreateListeningSocket(port);
    for (size_t i = 0u; i < threadCount; i++) {
        threadPool->Append(
            [this, stoken = StopSource_.get_token(), serverFd]() {
                HandleRequests(serverFd).Run();
            }
        );
    }
}

void THttpServer::TImpl::StopListen() {
    StopSource_.request_stop();
}

void THttpServer::TImpl::AddCallback(const THttpCallback& callback) {
    Callbacks_.emplace(callback.Target, callback);
}

// TODO: handle end of buffer
NAsync::TAsyncTask<bool> THttpServer::TImpl::HandleRequests(int serverFd) {
    while (true) try {
        auto client = co_await NUtils::AcceptTCPConnectionAsync(Reactor_, serverFd);

        std::vector<char> buffer(DEFAULT_BUFFER_SIZE);
        ssize_t headersSize = 0;
        size_t totalMsgSize = 0;
        while (true) {
            totalMsgSize += co_await NUtils::ReadTCPAsync(Reactor_, client, buffer, totalMsgSize);
            const ssize_t offset = std::max(headersSize - static_cast<ssize_t>(DCRLF.size()), 0l);
            std::string_view sv{buffer.begin(), buffer.end()};
            if (const auto eof = sv.find(DCRLF, offset); eof < buffer.size()) {
                headersSize = eof + offset + DCRLF.size();
                break;
            }
        }
        std::string_view sv{buffer.begin(), buffer.begin() + headersSize};

        THttpRequestPtr clientRequest = std::make_shared<THttpRequest>();
        const auto eosl = sv.find(CRLF);
        clientRequest->ParseStartLine(sv.substr(0, eosl));
        sv = sv.substr(eosl + CRLF.size());
        clientRequest->ParseHeaders(sv);
        const auto contentSize = clientRequest->GetHeader(CONTENT_LENGTH_HEADER);
        if (!contentSize.empty()) {
            size_t cLength = std::stoul(std::string(contentSize));
            while (totalMsgSize < headersSize + cLength) {
                totalMsgSize += co_await NUtils::ReadTCPAsync(Reactor_, client, buffer, totalMsgSize);
            }
            std::span<char> bufferSpan(buffer);
            clientRequest->SetContent(bufferSpan.subspan(headersSize, cLength));
            clientRequest->SetContentBuffer(std::move(buffer));
        }
        co_await ProcessRequest(clientRequest, client);
    } catch (const std::exception& e) {
        std::cerr << "Error handling request: " << e.what() << std::endl;
    }
    co_return true;
}

NAsync::TAsyncTask<bool> THttpServer::TImpl::ProcessRequest(const THttpRequestPtr request, int clientFd) {
    THttpResponse response;
    if (const auto callback = Callbacks_.find(request->GetTarget()); callback != Callbacks_.cend()) {
        response.SetStatus(EHttpStatus::OK);
    } else {
        std::string errorMessage = "Target not found";
        response.SetStatus(EHttpStatus::BAD_REQUEST);
        response.AddHeader("Content-Type", "text/plain");
        response.SetContent(errorMessage);
        response.SetContentBuffer(std::move(errorMessage));
        response.AddHeader(std::string(CONTENT_LENGTH_HEADER), std::to_string(errorMessage.size()));
    }
    const auto responseSpans = response.Serialize();
    co_await NUtils::WriteTCPAsync(Reactor_, clientFd, responseSpans[0]);
    co_await NUtils::WriteTCPAsync(Reactor_, clientFd, responseSpans[1]);
    close(clientFd);
    co_return true;
}

// TODO: handle end of buffer
NAsync::TAsyncTask<THttpResponsePtr> THttpServer::TImpl::SendRequest(const THttpRequestPtr request, std::string host, const uint16_t port) {
    const auto server = co_await NUtils::ConnectTCPHostAsync(Reactor_, host, port);
    const auto requestSpans = request->Serialize();
    co_await NUtils::WriteTCPAsync(Reactor_, server, requestSpans[0]);
    co_await NUtils::WriteTCPAsync(Reactor_, server, requestSpans[1]);

    std::vector<char> buffer(DEFAULT_BUFFER_SIZE);
    ssize_t headersSize = 0;
    size_t totalMsgSize = 0;
    while (true) {
        totalMsgSize += co_await NUtils::ReadTCPAsync(Reactor_, server, buffer, totalMsgSize);
        const ssize_t offset = std::max(headersSize - static_cast<ssize_t>(DCRLF.size()), 0l);
        std::string_view sv{buffer.begin(), buffer.end()};
        if (const auto eof = sv.find(DCRLF, offset); eof < buffer.size()) {
            headersSize = eof + offset + DCRLF.size();
            break;
        }
    }
    std::string_view sv{buffer.begin(), buffer.begin() + headersSize};

    THttpResponsePtr serverResponse = std::make_shared<THttpResponse>();
    const auto eosl = sv.find(CRLF);
    serverResponse->ParseStartLine(sv.substr(0, eosl));
    sv = sv.substr(eosl + CRLF.size());
    serverResponse->ParseHeaders(sv);
    const auto contentSize = serverResponse->GetHeader(CONTENT_LENGTH_HEADER);
    if (!contentSize.empty()) {
        size_t cLength = std::stoul(std::string(contentSize));
        while (totalMsgSize < headersSize + cLength) {
            totalMsgSize += co_await NUtils::ReadTCPAsync(Reactor_, server, buffer, totalMsgSize);
        }
        std::span<char> bufferSpan(buffer);
        buffer.resize(totalMsgSize);
        serverResponse->SetContent(bufferSpan.subspan(headersSize, cLength));
        serverResponse->SetContentBuffer(std::move(buffer));
    }
    co_return serverResponse;
}

NAsync::TAsyncTask<bool> THttpServer::TImpl::SendResponse(const THttpResponsePtr response, int clientFd) {
    const auto responseSpans = response->Serialize();
    co_await NUtils::WriteTCPAsync(Reactor_, clientFd, responseSpans[0]);
    co_await NUtils::WriteTCPAsync(Reactor_, clientFd, responseSpans[1]);
    close(clientFd);
    co_return true;
}


THttpServer::THttpServer(NAsync::TReactorPtr reactor)
    : Pimpl_(std::make_unique<TImpl>(reactor)) {}

THttpServer::~THttpServer() = default;

void THttpServer::Listen(NAsync::TThreadPoolPtr threadPool, const size_t threadCount, const uint16_t port) {
    Pimpl_->Listen(std::move(threadPool), threadCount, port);
}

void THttpServer::AddCallback(const THttpCallback& callback) {
    Pimpl_->AddCallback(callback);
}

void THttpServer::StopListen() {
    return Pimpl_->StopListen();
}

NAsync::TAsyncTask<THttpResponsePtr> THttpServer::SendRequest(const THttpRequestPtr request, std::string host, const uint16_t port) {
    return Pimpl_->SendRequest(std::move(request), std::move(host), port);
}

NAsync::TAsyncTask<bool> THttpServer::SendResponse(const THttpResponsePtr response, int clientFd) {
    return Pimpl_->SendResponse(std::move(response), clientFd);
}

} // namespace NHttp
