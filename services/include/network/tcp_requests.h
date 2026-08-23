#pragma once

#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <vector>

#include <include/network/fd_utils.h>
#include <include/async/async_task.h>
#include <include/async/reactor.h>

namespace NUtils {

NAsync::TAsyncTask<bool> SendFileAsync(NAsync::TReactorPtr reactor, int socket, std::string filePath);

NAsync::TAsyncTask<bool> WriteTCPAsync(NAsync::TReactorPtr reactor, int socket, const std::span<const char> data);
NAsync::TAsyncTask<bool> WriteTCPAsync(NAsync::TReactorPtr reactor, int socket, const std::vector<char>& data);
NAsync::TAsyncTask<bool> WriteTCPAsync(NAsync::TReactorPtr reactor, int socket, const std::string& data);

NAsync::TAsyncTask<size_t> ReadTCPAsync(NAsync::TReactorPtr reactor, int socket, std::span<char> buffer, const size_t offset);

NAsync::TAsyncTask<int> AcceptTCPConnectionAsync(NAsync::TReactorPtr reactor, int serverSocket);

NAsync::TAsyncTask<int> ConnectTCPHostAsync(NAsync::TReactorPtr reactor, std::string host, uint16_t port);

} // namespace NUtils
