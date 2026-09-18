#pragma once

#include <include/async/async_task.h>
#include <include/async/reactor.h>

namespace NUtils {

NAsync::TAsyncTask<bool> WriteFileAsync(NAsync::TReactorPtr reactor, int fd, const std::vector<char>& data, uint64_t offset = 0ull);

NAsync::TAsyncTask<bool> WriteFileAsync(NAsync::TReactorPtr reactor, int fd, const std::string& data, uint64_t offset = 0ull);

NAsync::TAsyncTask<std::string> ReadFileAsync(NAsync::TReactorPtr reactor, int fd, uint64_t offset = 0ull);

} // namespace NUtils
