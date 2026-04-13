#pragma once

#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

namespace NUtils {

struct TFdGuard {
    explicit TFdGuard(int fd) : Fd_(fd) {}

    TFdGuard(const TFdGuard&) = delete;
    TFdGuard& operator=(const TFdGuard&) = delete;
    TFdGuard(TFdGuard&&) = default;
    TFdGuard& operator=(TFdGuard&&) = default;

    void Close() {
        if (Fd_ >= 0) {
            close(Fd_);
            Fd_ = -1;
        }
    }

    ~TFdGuard() {
        Close();
    }
private:
    int Fd_ = -1;
};

inline int CreateListeningSocket(uint16_t port) {
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (fd < 0) return -1;
    
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    
    if (bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    
    if (listen(fd, 128) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

inline void SetSocketNonblocking(int socket) {
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags != -1) {
        fcntl(socket, F_SETFL, flags | O_NONBLOCK);
    }
}

} // namespace NUtils
