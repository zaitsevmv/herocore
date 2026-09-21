#pragma once

#include <memory>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace NRpc {

struct TRpcHeader {
    std::string_view Name;
    std::string_view Value;
};

using TContentBufferType = std::variant<std::string, std::vector<char>>;

class IRpcMessage {
public:
    virtual std::string Serialize() = 0;
    virtual void SetBuffer(std::string&& buffer) = 0;

private:
    std::vector<TRpcHeader> Headers_;
    TContentBufferType Buffer_;
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

IRpcMessagePtr Parse(std::string message);
IRpcMessagePtr Parse(std::vector<char> message);
IRpcMessagePtr Parse(const std::span<const char>& message);

} // namespace NRpc
