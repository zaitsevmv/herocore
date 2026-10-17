#include "rpc_message.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <string_view>

namespace NRpc {

namespace {

static constexpr std::string_view CRLF = "\n";
static constexpr std::string_view HEADER_SPLIT = "::";

template<class... Ts>
struct overloads : Ts... { using Ts::operator()...; };

const auto BufferVisitor = overloads
{
    [](const std::string& str){ return std::string_view(str); },
    [](const std::vector<char>& vec){ return std::string_view(vec.cbegin(), vec.cend()); }
};

void FillHeaders(IRpcMessage* msg) {
    std::string_view msgView = msg->Serialize();
    msgView = msgView.substr(1);
    while (true) {
        const auto eol = msgView.find_first_of(CRLF);
        if (eol >= msgView.size()) {
            break;
        }
        const auto headerSplit = msgView.find_first_of(HEADER_SPLIT);
        if (headerSplit >= msgView.size()) {
            throw;
        }
        TRpcHeader header{
            .Name = msgView.substr(0, headerSplit),
            .Value = msgView.substr(headerSplit + HEADER_SPLIT.size(), eol)
        };
        msg->AddHeader(header);
        msgView = msgView.substr(eol + CRLF.size());
    }
}

} // namespace

IRpcMessage::IRpcMessage(std::string&& buffer)
    :Buffer_(buffer), BodyBuffer_([](){ return "";}) {}

IRpcMessage::IRpcMessage(std::vector<char>&& buffer)
    :Buffer_(buffer), BodyBuffer_([](){ return "";}) {}

TRpcMessage::TRpcMessage(std::string&& buffer)
    : IRpcMessage(std::move(buffer)) {}

TRpcMessage::TRpcMessage(std::vector<char>&& buffer)
    : IRpcMessage(std::move(buffer)) {}

std::string_view TRpcMessage::Serialize() {
    return std::visit(BufferVisitor, Buffer_);
}

void TRpcMessage::AddHeader(const TRpcHeader& header) {
    Headers_.push_back(header);
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

TRpcStreamMessage::TRpcStreamMessage(std::string&& buffer) 
    : IRpcMessage(std::move(buffer)){}

TRpcStreamMessage::TRpcStreamMessage(std::vector<char>&& buffer)
    : IRpcMessage(std::move(buffer)){}

void TRpcStreamMessage::ParseChunk(std::string&& message) {
    Buffer_ = std::move(message);
    FillHeaders(this);
}

void TRpcStreamMessage::ParseChunk(std::vector<char>&& message) {
    Buffer_ = std::move(message);
    FillHeaders(this);
}

std::string_view TRpcStreamMessage::Serialize() {
    return std::visit(BufferVisitor, Buffer_);
}

void TRpcStreamMessage::AddHeader(const TRpcHeader& header) {
    Headers_.push_back(header);
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

IRpcMessagePtr Parse(std::string message) {
    assert(message.size() > 0);
    IRpcMessagePtr msg;
    switch (message[0]) {
        case '.': 
        {
            msg = std::make_unique<TRpcMessage>(std::move(message));
        }
        case ',':
        {
            msg = std::make_unique<TRpcStreamMessage>(std::move(message));
        }
        default:
        {
            assert(false);
        }
    };
    FillHeaders(msg.get());
    return msg;
}

} // namespace NRpc
