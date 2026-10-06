#pragma once

#include <memory>
#include <optional>
#include <span>
#include <string>
#include <variant>
#include <vector>
#include "include/thread_safe/lazy_value.h"

namespace NRpc {

struct TRpcHeader {
    std::string_view Name;
    std::string_view Value;
};

using TContentBufferType = std::variant<std::string, std::vector<char>>;

class IRpcMessage {
public:
    virtual std::string_view Serialize() = 0;
    virtual void SetBuffer(std::string&& buffer) = 0;
    virtual std::optional<TRpcHeader> GetHeader(const std::string& key) const = 0;
    virtual std::string_view GetBody(const bool decode) const = 0;

protected:
    std::vector<TRpcHeader> Headers_;
    std::string_view Body_;
    TContentBufferType Buffer_;
    mutable Lazy<std::string> BodyBuffer_;
};
using IRpcMessagePtr = std::unique_ptr<IRpcMessage>;

class TRpcMessage : public IRpcMessage {
public:
    std::string_view Serialize() override;
    std::optional<TRpcHeader> GetHeader(const std::string& key) const override;
    std::string_view GetBody(const bool decode) const override;
};

enum class EStreamState : uint8_t {
    Opened = 0u,
    Closed = 1u
};

class TRpcStreamMessage : public IRpcMessage {
public:
    std::string_view Serialize() override;
    std::optional<TRpcHeader> GetHeader(const std::string& key) const override;
    std::string_view GetBody(const bool decode) const override;

    void ParseChunk(const std::span<const char>& message);
    EStreamState GetState() noexcept;

private:
    EStreamState StreamState_ = EStreamState::Closed;
};

IRpcMessagePtr Parse(std::string message);
IRpcMessagePtr Parse(std::vector<char> message);
IRpcMessagePtr Parse(const std::span<const char>& message);

} // namespace NRpc
