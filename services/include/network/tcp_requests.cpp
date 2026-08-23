#include "tcp_requests.h"

#include <arpa/inet.h>

#include <cstdint>
#include <cstring>
#include <fstream>
#include <span>
#include <string>
#include <vector>

#include "tcp_awaiters.h"
#include <include/network/fd_utils.h>
#include <include/async/async_task.h>
#include <include/async/reactor.h>

namespace {

NAsync::TAsyncTask<bool> WriteAllAsync(NAsync::TReactorPtr reactor, int socket, std::span<const char> buffer) {
    size_t sent = 0;
    while (sent < buffer.size()) {
        auto n = co_await NAsync::TTCPWriteAwaiter(reactor, socket, buffer.subspan(sent));
        if (n <= 0)
            co_return false;

        sent += n;
    }
    co_return true;
}

NAsync::TAsyncTask<bool> ReadExactAsync(NAsync::TReactorPtr reactor, int socket, std::span<char> buffer) {
    size_t received = 0;
    while (received < buffer.size()) {
        auto n = co_await NAsync::TTCPReadAwaiter(reactor, socket, buffer.subspan(received));
        if (n <= 0) co_return false;
        received += n;
    }
    co_return true;
}

} // namespace

namespace NUtils {

NAsync::TAsyncTask<bool> WriteTCPAsync(NAsync::TReactorPtr reactor, int socket, const std::span<const char> data) {
    co_return co_await WriteAllAsync(reactor, socket, data);
}

NAsync::TAsyncTask<bool> WriteTCPAsync(NAsync::TReactorPtr reactor, int socket, const std::vector<char>& data) {
    co_return co_await WriteAllAsync(reactor, socket, data);
}

NAsync::TAsyncTask<bool> WriteTCPAsync(NAsync::TReactorPtr reactor, int socket, const std::string& data) {
    co_return co_await WriteAllAsync(reactor, socket, data);
}

NAsync::TAsyncTask<size_t> ReadTCPAsync(NAsync::TReactorPtr reactor, int socket, std::span<char> buffer, size_t offset) {
    co_return co_await NAsync::TTCPReadAwaiter(reactor, socket, buffer.subspan(offset));
}

NAsync::TAsyncTask<int> AcceptTCPConnectionAsync(NAsync::TReactorPtr reactor, int serverSocket) {
    co_return co_await NAsync::TTCPAcceptAwaiter(reactor, serverSocket);
}

NAsync::TAsyncTask<int> ConnectTCPHostAsync(NAsync::TReactorPtr reactor, std::string host, uint16_t port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
        throw std::invalid_argument("Invalid address");
    }
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    NUtils::SetSocketNonblocking(sock);
    co_await NAsync::TTCPConnectAwaiter(reactor, sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    co_return sock;
}

} // namespace NUtils
