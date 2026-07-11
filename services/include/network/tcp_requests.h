#pragma once

#include "tcp_awaiters.h"
#include "network/fd_utils.h"
#include "messages/messages.h"
#include "async/async_task.h"
#include "async/reactor.h"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <span>
#include <string>
#include <vector>
#include <arpa/inet.h>

namespace {

inline NAsync::TAsyncTask<bool> WriteAllAsync(NAsync::TReactorPtr reactor, int socket, std::span<char> buffer) {
    size_t sent = 0;
    while (sent < buffer.size()) {
        auto n = co_await NAsync::TTCPWriteAwaiter(reactor, socket, buffer.subspan(sent));
        if (n <= 0)
            co_return false;

        sent += n;
    }
    co_return true;
}

inline NAsync::TAsyncTask<bool> ReadExactAsync(NAsync::TReactorPtr reactor, int socket, std::span<char> buffer) {
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

inline NAsync::TAsyncTask<bool> SendDataAsync(NAsync::TReactorPtr reactor, int socket, const std::vector<char>& data) {
    TMessage msg;
    msg.TotalSize = data.size();
    msg.ChunkSize = data.size();
    msg.Data = data;

    auto serialized = msg.Serialize();
    co_return co_await WriteAllAsync(reactor, socket, std::span<char>(serialized.data(), serialized.size()));
}

inline NAsync::TAsyncTask<bool> SendDataAsync(NAsync::TReactorPtr reactor, int socket, const std::string& data) {
    return SendDataAsync(reactor, socket, std::vector<char>(data.cbegin(), data.cend()));
}

inline NAsync::TAsyncTask<bool> SendFileAsync(NAsync::TReactorPtr reactor, int socket, std::string filePath) {
    std::ifstream fileStream(filePath, std::ios::binary | std::ios::ate);
    if (!fileStream.is_open()) {
        co_return false;
    }
    auto fileSize = fileStream.tellg();
    if (fileSize < 0) {
        co_return false;
    }

    fileStream.seekg(0, std::ios::beg);

    TMessage msg;
    msg.TotalSize = static_cast<uint32_t>(fileSize);

    std::vector<char> buffer(TCP_BUFFER_SIZE);
    size_t dbg = 0;
    while (fileStream) {
        fileStream.read(buffer.data(), TCP_BUFFER_SIZE - TMessage::GetServiceDataSize());
        std::streamsize bytesRead = fileStream.gcount();
        dbg += bytesRead;

        if (bytesRead > 0) {
            msg.Data.assign(buffer.data(), buffer.data() + bytesRead);
            msg.ChunkSize = bytesRead;
            
            auto serialized = msg.Serialize();
            if (!co_await WriteAllAsync(reactor, socket, std::span<char>(serialized.data(), serialized.size()))) {
                co_return false;
            }
        } else if (fileStream.fail() && !fileStream.eof()) {
            co_return false;
        }
    }
    co_return true;
}

inline NAsync::TAsyncTask<TMessage> ReadMessageAsync(NAsync::TReactorPtr reactor, int socket) {
    uint32_t header[2];

    if (!co_await ReadExactAsync(reactor, socket, std::span<char>((char*)header, sizeof(header)))) {
        throw std::runtime_error("header read failed");
    }

    uint32_t totalSize = ntohl(header[0]);
    uint32_t chunkSize = ntohl(header[1]);

    std::vector<char> data(chunkSize);
    if (!co_await ReadExactAsync(reactor, socket, std::span<char>(data.data(), data.size()))) {
        throw std::runtime_error("body read failed");
    }

    TMessage msg;
    msg.TotalSize = totalSize;
    msg.ChunkSize = chunkSize;
    msg.Data = std::move(data);
    co_return msg;
}

inline NAsync::TAsyncTask<int> AcceptConnectionAsync(NAsync::TReactorPtr reactor, int serverSocket) {
    co_return co_await NAsync::TTCPAcceptAwaiter(reactor, serverSocket);
}

inline NAsync::TAsyncTask<int> ConnectHostAsync(NAsync::TReactorPtr reactor, std::string host, uint16_t port) {
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
