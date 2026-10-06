#include "rpc_message.h"

#include <algorithm>

namespace NRpc {

namespace {

static constexpr std::string_view CRLF = "\n";

template<class... Ts>
struct overloads : Ts... { using Ts::operator()...; };

const auto BufferVisitor = overloads
{
    [](const std::string& str){ return std::string_view(str); },
    [](const std::vector<char>& vec){ return std::string_view(vec.cbegin(), vec.cend()); }
};

} // namespace

std::string_view TRpcMessage::Serialize() {
    return std::visit(BufferVisitor, Buffer_);
}

std::optional<TRpcHeader> TRpcMessage::GetHeader(const std::string& key) const {
    const auto it = std::ranges::find(Headers_, key, &TRpcHeader::Name);
    if (it == Headers_.cend()) {
        return std::nullopt;
    }
    return *it;
}

std::string_view TRpcMessage::GetBody(const bool decode) const {
    if (!decode) {
        return Body_;
    }
    return BodyBuffer_.Get();
}


void TRpcStreamMessage::ParseChunk(const std::span<const char>& message) {

}

std::string_view TRpcStreamMessage::Serialize() {
    return std::visit(BufferVisitor, Buffer_);
}

std::optional<TRpcHeader> TRpcStreamMessage::GetHeader(const std::string& key) const {
    const auto it = std::ranges::find(Headers_, key, &TRpcHeader::Name);
    if (it == Headers_.cend()) {
        return std::nullopt;
    }
    return *it;
}

std::string_view TRpcStreamMessage::GetBody(const bool decode) const {
    if (!decode) {
        return Body_;
    }
    return BodyBuffer_.Get();
}

EStreamState TRpcStreamMessage::GetState() noexcept {
    return StreamState_;
}

} // namespace NRpc

using namespace NRpc;

IRpcMessagePtr Parse(std::string message) {
    
}

IRpcMessagePtr Parse(const std::span<const char>& message) {

}
