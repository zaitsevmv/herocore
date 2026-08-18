#pragma once

#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <vector>

#include "files_awaiters.h"
#include <include/messages/messages.h>
#include <include/async/async_task.h>
#include <include/async/reactor.h>

namespace {

inline NAsync::TAsyncTask<bool> WriteAllAsync(NAsync::TReactorPtr reactor, int fd, std::span<const char> buffer, uint64_t offset) {
    size_t sent = 0;
    while (sent < buffer.size()) {
        auto n = co_await NAsync::TFilesWriteAwaiter(reactor, fd, buffer.subspan(sent), offset + sent);
        if (n <= 0)
            co_return false;

        sent += n;
    }
    co_return true;
}

inline NAsync::TAsyncTask<bool> ReadExactAsync(NAsync::TReactorPtr reactor, int fd, std::span<char> buffer, uint64_t offset) {
    size_t received = 0;
    while (received < buffer.size()) {
        auto n = co_await NAsync::TFilesReadAwaiter(reactor, fd, buffer.subspan(received), offset);
        if (n <= 0) co_return false;
        received += n;
    }
    co_return true;
}

} // namespace

namespace NUtils {

inline NAsync::TAsyncTask<bool> WriteFileAsync(NAsync::TReactorPtr reactor, int fd, const std::vector<char>& data, uint64_t offset = 0ull) {
    co_return co_await WriteAllAsync(reactor, fd, std::span<const char>(data.data(), data.size()), offset);
}

inline NAsync::TAsyncTask<bool> WriteFileAsync(NAsync::TReactorPtr reactor, int fd, const std::string& data, uint64_t offset = 0ull) {
    co_return co_await WriteAllAsync(reactor, fd, std::span<const char>(data.data(), data.size()), offset);
}

inline NAsync::TAsyncTask<std::string> ReadFileAsync(NAsync::TReactorPtr reactor, int fd, uint64_t offset = 0ull) {
    std::vector<char> data(FILES_BUFFER_SIZE);
    if (!co_await ReadExactAsync(reactor, fd, std::span<char>(data.data(), data.size()), offset)) {
        throw std::runtime_error("body read failed");
    }
    co_return std::string(data.cbegin(), data.cend());
}

} // namespace NUtils
