#pragma once

#include <memory>
#include <span>
#include <string>

namespace NRpc {

class IRpcMessage {
public:
    virtual std::string Serialize() = 0;
};
using IRpcMessagePtr = std::unique_ptr<IRpcMessage>;

class TRpcMessage : public IRpcMessage {
public:
    std::string Serialize() override;
};

enum class EStreamState : uint8_t {
    Opened = 0u,
    Closed = 1u
};

class TRpcStreamMessage : public IRpcMessage {
public:
    std::string Serialize() override;

    void ParseChunk(const std::span<const char>& message);
    EStreamState GetState() noexcept;

private:
    EStreamState StreamState_ = EStreamState::Closed;
};

IRpcMessagePtr Parse(const std::string& message);
IRpcMessagePtr Parse(const std::span<const char>& message);

} // namespace NRpc
