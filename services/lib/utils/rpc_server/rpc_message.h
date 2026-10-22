#pragma once

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <lib/base/thread_safe/lazy_value.h>

namespace NRpc {

struct TRpcHeader {
    std::string_view Name;
    std::string_view Value;
};

using TContentBufferType = std::variant<std::string, std::vector<char>>;

class IRpcMessage {
public:
    IRpcMessage(std::string&& buffer);
    IRpcMessage(std::vector<char>&& buffer);
    virtual ~IRpcMessage();

    virtual std::string_view Serialize() = 0;
    virtual void AddHeader(const TRpcHeader& header) = 0;
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
    TRpcMessage(std::string&& buffer);
    TRpcMessage(std::vector<char>&& buffer);

    std::string_view Serialize() override;
    void AddHeader(const TRpcHeader& header) override;
    std::optional<TRpcHeader> GetHeader(const std::string& key) const override;
    std::string_view GetBody(const bool decode) const override;
};

enum class EStreamState : uint8_t {
    Opened = 0u,
    Closed = 1u
};

class TRpcStreamMessage : public IRpcMessage {
public:
    TRpcStreamMessage(std::string&& buffer);
    TRpcStreamMessage(std::vector<char>&& buffer);

    std::string_view Serialize() override;
    void AddHeader(const TRpcHeader& header) override;
    std::optional<TRpcHeader> GetHeader(const std::string& key) const override;
    std::string_view GetBody(const bool decode) const override;

    void ParseChunk(std::string&& message);
    void ParseChunk(std::vector<char>&& message);
    EStreamState GetState() noexcept;

private:
    EStreamState StreamState_ = EStreamState::Closed;
};

IRpcMessagePtr Parse(std::string message);
IRpcMessagePtr Parse(std::vector<char> message);

} // namespace NRpc
